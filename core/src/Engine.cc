#include <iostream>
#include <cmath>
#include <algorithm>

#include <boost/algorithm/clamp.hpp>
#include <boost/numeric/odeint/iterator/n_step_iterator.hpp>

#include "pinocchio/parsers/urdf.hpp"
#include "pinocchio/algorithm/kinematics.hpp"
#include "pinocchio/algorithm/frames.hpp"
#include "pinocchio/multibody/model.hpp"
#include "pinocchio/algorithm/aba.hpp"
#include "pinocchio/algorithm/rnea.hpp"
#include "pinocchio/algorithm/joint-configuration.hpp"
#include "pinocchio/algorithm/energy.hpp"

#include "exo_simu/core/Utilities.h"
#include "exo_simu/core/TelemetryData.h"
#include "exo_simu/core/TelemetryRecorder.h"
#include "exo_simu/core/AbstractController.h"
#include "exo_simu/core/AbstractSensor.h"
#include "exo_simu/core/Model.h"
#include "exo_simu/core/Engine.h"


namespace exo_simu
{
    Engine::Engine(void):
    engineOptions_(nullptr),
    isInitialized_(false),
    model_(nullptr),
    controller_(nullptr),
    engineOptionsHolder_(),
    callbackFct_([](float64_t const & t,
                    vectorN_t const & x) -> bool
                 {
                     return true;
                 }),
    telemetrySender_(),
    telemetryData_(nullptr),
    telemetryRecorder_(nullptr),
    stepperState_()
    {
        telemetryData_ = std::make_shared<TelemetryData>();
        telemetryRecorder_ = std::make_unique<TelemetryRecorder>(std::const_pointer_cast<TelemetryData const>(telemetryData_));

        setOptions(getDefaultOptions());
    }

    Engine::~Engine(void)
    {
        // Empty
    }

    result_t Engine::initialize(Model              & model,
                                AbstractController & controller,
                                callbackFct_t        callbackFct)
    {
        result_t returnCode = result_t::SUCCESS;

        if (!model.getIsInitialized())
        {
            std::cout << "Error - Engine::initialize - Model not initialized." << std::endl;
            return result_t::ERROR_INIT_FAILED;
        }
        model_ = &model;

        if (returnCode == result_t::SUCCESS)
        {
            returnCode = stepperState_.initialize(*model_, vectorN_t::Zero(model_->nx()));
        }

        if (!controller.getIsInitialized())
        {
            std::cout << "Error - Engine::initialize - Controller not initialized." << std::endl;
            return result_t::ERROR_INIT_FAILED;
        }
        if (returnCode == result_t::SUCCESS)
        {
            try
            {
                float64_t t = 0;
                vectorN_t q = vectorN_t::Zero(model_->nq());
                vectorN_t v = vectorN_t::Zero(model_->nv());
                vectorN_t uCommand = vectorN_t::Zero(model_->getJointsVelocityIdx().size());
                vectorN_t uInternal = vectorN_t::Zero(model_->nv());
                controller.compute_command(*model_, t, q, v, uCommand);
                if(uCommand.size() != (int32_t) model_->getJointsVelocityIdx().size())
                {
                    std::cout << "Error - Engine::initialize - The controller's method 'compute_command' returns command with wrong size." << std::endl;
                    returnCode = result_t::ERROR_BAD_INPUT;
                }
                controller.internalDynamics(*model_, t, q, v, uInternal);
                if(uInternal.size() != model_->nv())
                {
                    std::cout << "Error - Engine::initialize - The controller's method 'internalDynamics' returns command with wrong size." << std::endl;
                    returnCode = result_t::ERROR_BAD_INPUT;
                }
            }
            catch (std::exception& e)
            {
                std::cout << "Error - Engine::initialize - Something is wrong with the Controller. Impossible to compute command." << std::endl;
                returnCode = result_t::ERROR_GENERIC;
            }
        }
        if (returnCode == result_t::SUCCESS)
        {
            controller_ = &controller;
        }

        if (returnCode == result_t::SUCCESS)
        {
            // TODO: Check that the callback function is working as expected
            callbackFct_ = callbackFct;
        }

        if (returnCode == result_t::SUCCESS)
        {
            isInitialized_ = true;
            setOptions(engineOptionsHolder_); // Make sure the gravity is properly set at model level
        }

        // Initialize the logger
        stepperState_.initialize(*model_);
        telemetryData_->reset();
        telemetrySender_.configureObject(telemetryData_, ENGINE_OBJECT_NAME);
        if (engineOptions_->telemetry.logConfiguration)
        {
            (void) registerNewVectorEntry(telemetrySender_, stepperState_.qNames, stepperState_.qLast);
        }
        if (engineOptions_->telemetry.logVelocity)
        {
            (void) registerNewVectorEntry(telemetrySender_, stepperState_.vNames, stepperState_.vLast);
        }
        if (engineOptions_->telemetry.logAcceleration)
        {
            (void) registerNewVectorEntry(telemetrySender_, stepperState_.aNames, stepperState_.aLast);
        }
        if (engineOptions_->telemetry.logCommand)
        {
            (void) registerNewVectorEntry(telemetrySender_, stepperState_.uCommandNames, stepperState_.uCommandLast);
        }
        telemetrySender_.registerNewEntry<float64_t>("energy", 0.0);
        model_->configureTelemetry(telemetryData_);
        telemetryRecorder_->initialize();

        return returnCode;
    }

    result_t Engine::simulate(vectorN_t const & x_init,
                              float64_t const & end_time)
    {
        result_t returnCode = result_t::SUCCESS;

        if(!isInitialized_)
        {
            std::cout << "Error - Engine::simulate - Engine not initialized. Impossible to run the simulation." << std::endl;
            return result_t::ERROR_INIT_FAILED;
        }

        if(x_init.rows() != model_->nx())
        {
            std::cout << "Error - Engine::simulate - Size of x_init (" << x_init.size() << ") inconsistent with model size (" << model_->nx() << ")." << std::endl;
            return result_t::ERROR_BAD_INPUT;
        }

        if(end_time < 5e-2)
        {
            std::cout << "Error - Engine::simulate - The duration of the simulation cannot be shorter than 50ms." << std::endl;
            return result_t::ERROR_BAD_INPUT;
        }

        // Define the stepper iterators
        auto rhsBind = bind(&Engine::systemDynamics, this,
                            std::placeholders::_3,
                            std::placeholders::_1,
                            std::placeholders::_2);
        auto stepper = make_controlled(engineOptions_->stepper.tolAbs,
                                       engineOptions_->stepper.tolRel,
                                       stepper_t());

        // initialize the random number generators
        resetRandGenerators(engineOptions_->stepper.randomSeed);
        model_->reset();
        controller_->reset();
        
        // Initialize the logger, model, and stepper internal state
        model_->pncData_ = pinocchio::Data(model_->pncModel_);
        stepperState_.initialize(*model_, x_init);
        systemDynamics(0, stepperState_.x, stepperState_.dxdt);
        telemetryRecorder_->initialize();

        // Compute the breakpoints' period (for command or observation) during the integration loop
        float64_t updatePeriod = 0.0;
        if (engineOptions_->stepper.sensorsUpdatePeriod < std::numeric_limits<float64_t>::epsilon())
        {
            updatePeriod = engineOptions_->stepper.controllerUpdatePeriod;
        }
        else if (engineOptions_->stepper.controllerUpdatePeriod < std::numeric_limits<float64_t>::epsilon())
        {
            updatePeriod = engineOptions_->stepper.sensorsUpdatePeriod;
        }
        else
        {
            updatePeriod = std::min(engineOptions_->stepper.sensorsUpdatePeriod,
                                    engineOptions_->stepper.controllerUpdatePeriod);
        }

        // Set the initial time step
        float64_t dt = 0.0;
        if (updatePeriod > 0)
        {
            dt = updatePeriod; // The initial time step is the update period
        }
        else
        {
            dt = 5e-4;
        }

        // Integration loop based on boost::numeric::odeint::detail::integrate_times
        float64_t current_time = 0.0;
        float64_t next_time = 0.0;
        failed_step_checker fail_checker; // to throw a runtime_error if step size adjustment fail
        while (true)
        {
            // Log the current time, state, command, and sensors
            if (engineOptions_->telemetry.logConfiguration)
            {
                updateVectorValue(telemetrySender_, stepperState_.qNames, stepperState_.qLast);
            }
            if (engineOptions_->telemetry.logVelocity)
            {
                updateVectorValue(telemetrySender_, stepperState_.vNames, stepperState_.vLast);
            }
            if (engineOptions_->telemetry.logAcceleration)
            {
                updateVectorValue(telemetrySender_, stepperState_.aNames, stepperState_.aLast);
            }
            if (engineOptions_->telemetry.logCommand)
            {
                updateVectorValue(telemetrySender_, stepperState_.uCommandNames, stepperState_.uCommandLast);
            }
            telemetrySender_.updateValue<float64_t>("energy", stepperState_.energyLast);
            model_->updateSensorsTelemetry();
            telemetryRecorder_->flushDataSnapshot(stepperState_.tLast);

            /* Stop the simulation if the end time has been reached, if
               the callback returns false, or if the number of integration
               steps exceeds 1e5. */
            if (std::abs(end_time - current_time) < std::numeric_limits<float64_t>::epsilon()
            || !callbackFct_(current_time, stepperState_.x)
            || stepperState_.iterLast >= 1e5)
            {
                break;
            }

            if (updatePeriod > 0)
            {
                // Get the current time and target time at next iteration
                current_time = next_time;
                next_time += std::min(updatePeriod, end_time - current_time); // Make sure it ends exactly at the end_time

                // Update the sensor data if necessary (only for finite update frequency)
                if (engineOptions_->stepper.sensorsUpdatePeriod > 0)
                {
                    float64_t next_time_update_sensor = std::round(current_time / engineOptions_->stepper.sensorsUpdatePeriod) *
                        engineOptions_->stepper.sensorsUpdatePeriod;
                    if (std::abs(current_time - next_time_update_sensor) < 1e-8)
                    {
                        model_->setSensorsData(stepperState_.tLast,
                                               stepperState_.qLast,
                                               stepperState_.vLast,
                                               stepperState_.aLast,
                                               stepperState_.uLast);
                    }
                }

                // Update the controller command if necessary (only for finite update frequency)
                if (engineOptions_->stepper.controllerUpdatePeriod > 0)
                {
                    float64_t next_time_update_controller = std::round(current_time / engineOptions_->stepper.controllerUpdatePeriod) *
                        engineOptions_->stepper.controllerUpdatePeriod;
                    if (std::abs(current_time - next_time_update_controller) < 1e-8)
                    {
                        controller_->compute_command(*model_,
                                                     stepperState_.tLast,
                                                     stepperState_.qLast,
                                                     stepperState_.vLast,
                                                     stepperState_.uCommandLast);
                        std::vector<int32_t> jointsVelocityIdx = model_->getJointsVelocityIdx();
                        for (uint32_t i=0; i < jointsVelocityIdx.size(); i++)
                        {
                            uint32_t jointId = jointsVelocityIdx[i];
                            float64_t torque_max = model_->pncModel_.effortLimit(jointId); // effortLimit is given in the velocity vector space
                            stepperState_.uCommandLast[i] = boost::algorithm::clamp(stepperState_.uCommandLast[i], -torque_max, torque_max);
                            stepperState_.uControl[jointId] = stepperState_.uCommandLast[i];
                        }
                        systemDynamics(current_time, stepperState_.x, stepperState_.dxdt); // Update the internal stepper state dxdt since the dynamics has changed.
                    }
                }

                // Compute the next step using adaptive step method
                while (current_time < next_time)
                {
                    // adjust stepsize to end up exactly at the next breakpoint
                    float64_t current_dt = std::min(dt, next_time - current_time);
                    if (success == stepper.try_step(rhsBind, stepperState_.x, stepperState_.dxdt, current_time, current_dt))
                    {
                        fail_checker.reset(); // reset the fail counter, see #173
                        dt = std::max(dt, current_dt); // continue with the original step size if dt was reduced due to the next breakpoint
                    }
                    else
                    {
                        fail_checker();  // check for possible overflow of failed steps in step size adjustment
                        dt = current_dt;
                    }
                }
            }
            else
            {
                // Compute the next step using adaptive step method
                dt = std::min(dt, end_time - current_time); // Make sure it ends exactly at the end_time
                controlled_step_result res = fail;
                while (res == fail)
                {
                    res = stepper.try_step(rhsBind, stepperState_.x, stepperState_.dxdt, current_time, dt);
                    if (res == success)
                    {
                        fail_checker.reset(); // reset the fail counter, see #173
                    }
                    else
                    {
                        fail_checker();  // check for possible overflow of failed steps in step size adjustment
                    }
                }
            }

            // Update internal state of the stepper
            vectorN_t const & q = stepperState_.x.head(model_->nq());
            vectorN_t const & v = stepperState_.x.tail(model_->nv());
            vectorN_t const & a = stepperState_.dxdt.tail(model_->nv());
            stepperState_.uLast = pinocchio::rnea(model_->pncModel_, model_->pncData_, q, v, a);
            // Get system energy, kinematic computation are not needed since they were already done in RNEA.
            float64_t energy =  pinocchio::kineticEnergy(model_->pncModel_, model_->pncData_, q, v, false) +
                                pinocchio::potentialEnergy(model_->pncModel_, model_->pncData_, q, false);
            stepperState_.updateLast(current_time, q, v, a, stepperState_.uLast, stepperState_.uCommandLast, energy); // uLast and uCommandLast are already up-to-date
        }

        return returnCode;
    }

    void Engine::systemDynamics(float64_t const & t,
                                vectorN_t const & x,
                                vectorN_t       & dxdt)
    {
        /* Note that the position of the free flyer is in world frame, whereas the
           velocities and accelerations are relative to the parent body frame. */

        // Extract configuration and velocity vectors
        vectorN_t const & q = x.head(model_->nq());
        vectorN_t const & v = x.tail(model_->nv());

        // Compute kinematics information
        pinocchio::forwardKinematics(model_->pncModel_, model_->pncData_, q, v);
        pinocchio::framesForwardKinematics(model_->pncModel_, model_->pncData_);

        // Compute the external forces
        pinocchio::container::aligned_vector<pinocchio::Force> fext(model_->pncModel_.joints.size(),
                                                                    pinocchio::Force::Zero());
        std::vector<int32_t> const & contactFramesIdx = model_->getContactFramesIdx();
        for(uint32_t i=0; i < contactFramesIdx.size(); i++)
        {
            int32_t const & contactFrameIdx = contactFramesIdx[i];
            model_->contactForces_[i] = pinocchio::Force(contactDynamics(contactFrameIdx));
            int32_t parentIdx = model_->pncModel_.frames[contactFrameIdx].parent;
            fext[parentIdx] += model_->contactForces_[i];
        }

        // Update the sensor data if necessary (only for infinite update frequency)
        if (engineOptions_->stepper.sensorsUpdatePeriod < std::numeric_limits<float64_t>::epsilon())
        {
            model_->setSensorsData(t, q, v, stepperState_.aLast, stepperState_.uLast); // Impossible to have access to the current acceleration and efforts
        }

        // Update the controller command if necessary (only for infinite update frequency)
        if (engineOptions_->stepper.controllerUpdatePeriod < std::numeric_limits<float64_t>::epsilon())
        {
            controller_->compute_command(*model_, t, q, v, stepperState_.uCommandLast); // Be careful, in this particular case uCommandLast is not guarantee to be the last command
            std::vector<int32_t> jointsVelocityIdx = model_->getJointsVelocityIdx();
            for (uint32_t i=0; i < jointsVelocityIdx.size(); i++)
            {
                uint32_t jointId = jointsVelocityIdx[i];
                float64_t torque_max = model_->pncModel_.effortLimit(jointId); // effortLimit is given in the velocity vector space
                stepperState_.uCommandLast[i] = boost::algorithm::clamp(stepperState_.uCommandLast[i], -torque_max, torque_max);
                stepperState_.uControl[jointId] = stepperState_.uCommandLast[i];
            }
        }

        // Compute command and internal dynamics
        controller_->internalDynamics(*model_, t, q, v, stepperState_.uInternal); // TODO: Send the values at previous iteration instead
        boundsDynamics(q, v, stepperState_.uBounds);
        vectorN_t u = stepperState_.uBounds + stepperState_.uInternal + stepperState_.uControl;

        // Compute dynamics
        vectorN_t a = pinocchio::aba(model_->pncModel_, model_->pncData_, q, v, u, fext);

        /* Hack to compute the configuration vector derivative, including the
           quaternions on SO3 automatically. Note that the time difference must
           not be too small to avoid failure. Note that pinocchio::integrate is
           quite slow (more than 5ns, compare to 85ns for pinocchio::aba). */
        float64_t dt = std::max(1e-5, t - stepperState_.tLast);
        vectorN_t qNext = vectorN_t::Zero(model_->nq());
        pinocchio::integrate(model_->pncModel_, q, v*dt, qNext);
        vectorN_t qDot = (qNext - q) / dt;

        // Fill up dxdt
        dxdt.resize(model_->nx());
        dxdt.head(model_->nq()) = qDot;
        dxdt.tail(model_->nv()) = a;
    }

    vectorN_t Engine::contactDynamics(int32_t const & frameId) const
    {
        // /* /!\ Note that the contact dynamics depends only on kinematics data. /!\ */

        contactOptions_t const * const contactOptions_ = &engineOptions_->contacts;

        Eigen::Matrix3d const & tformFrameRot = model_->pncData_.oMf[frameId].rotation();
        Eigen::Vector3d const & posFrame = model_->pncData_.oMf[frameId].translation();

        vectorN_t fextLocal = vectorN_t::Zero(6);

        if(posFrame(2) < 0.0)
        {
            // Initialize the contact force
            Eigen::Vector3d fextInWorld(0.0, 0.0, 0.0);

            // Get various transformations
            Eigen::Matrix3d const & tformFrameJointRot = model_->pncModel_.frames[frameId].placement.rotation();
            Eigen::Vector3d const & posFrameJoint = model_->pncModel_.frames[frameId].placement.translation();

            Eigen::Vector3d motionFrame = pinocchio::getFrameVelocity(model_->pncModel_,
                                                                      model_->pncData_,
                                                                      frameId).linear();
            Eigen::Vector3d vFrameInWorld = tformFrameRot * motionFrame;

            // Compute normal force
            float64_t damping = 0;
            if(vFrameInWorld(2) < 0)
            {
                damping = -contactOptions_->damping * vFrameInWorld(2);
            }
            fextInWorld(2) = -contactOptions_->stiffness * posFrame(2) + damping;

            // Compute friction forces
            Eigen::Vector2d const & vxy = vFrameInWorld.head<2>();
            float64_t vNorm = vxy.norm();
            float64_t frictionCoeff;
            if(vNorm > contactOptions_->dryFrictionVelEps)
            {
                if(vNorm < 1.5 * contactOptions_->dryFrictionVelEps)
                {
                    frictionCoeff = -2.0 * vNorm * (contactOptions_->frictionDry - contactOptions_->frictionViscous) \
                        / contactOptions_->dryFrictionVelEps + 3.0*contactOptions_->frictionDry - \
                        2.0*contactOptions_->frictionViscous;
                }
                else
                {
                    frictionCoeff = contactOptions_->frictionViscous;
                }
            }
            else
            {
                frictionCoeff = vNorm * contactOptions_->frictionDry / contactOptions_->dryFrictionVelEps;
            }
            fextInWorld.head<2>() = -vxy * frictionCoeff * fextInWorld(2);

            // Make sure that the tangential force never exceeds 1e5 N for the sake of numerical stability
            fextInWorld.head<2>() = fextInWorld.head<2>().unaryExpr([](float64_t x) -> float64_t
                                                                    {
                                                                        return std::min(std::max(x, -1e5), 1e5);
                                                                    });

            // Compute the forces at the origin of the parent joint frame
            fextLocal.head<3>() = tformFrameJointRot * tformFrameRot.transpose() * fextInWorld;
            fextLocal.tail<3>() = posFrameJoint.cross(fextLocal.head<3>());

            // Add blending factor
            float64_t blendingFactor = -posFrame(2) / contactOptions_->transitionEps;
            float64_t blendingLaw = std::tanh(2 * blendingFactor);
            fextLocal *= blendingLaw;
        }

        return fextLocal;
    }

    void Engine::boundsDynamics(vectorN_t const & q,
                                vectorN_t const & v,
                                vectorN_t       & u)
    {
        // Enforce the bounds of the actuated joints of the model
        u = vectorN_t::Zero(model_->nv());

        Model::jointOptions_t const & mdlJointOptions_ = model_->mdlOptions_->joints;
        Engine::jointOptions_t const & engineJointOptions_ = engineOptions_->joints;

        std::vector<int32_t> jointsPositionIdx = model_->getJointsPositionIdx();
        std::vector<int32_t> jointsVelocityIdx = model_->getJointsVelocityIdx();
        for (uint32_t i = 0; i < jointsPositionIdx.size(); i++)
        {
            float64_t const qJoint = q(jointsPositionIdx[i]);
            float64_t const vJoint = v(jointsVelocityIdx[i]);
            float64_t const qJointMin = mdlJointOptions_.boundsMin(i);
            float64_t const qJointMax = mdlJointOptions_.boundsMax(i);

            float64_t forceJoint = 0;
            float64_t qJointError = 0;
            if (qJoint > qJointMax)
            {
                qJointError = qJoint - qJointMax;
                float64_t damping = -engineJointOptions_.boundDamping * std::max(vJoint, 0.0);
                forceJoint = -engineJointOptions_.boundStiffness * qJointError + damping;
            }
            else if (qJoint < qJointMin)
            {
                qJointError = qJointMin - qJoint;
                float64_t damping = -engineJointOptions_.boundDamping * std::min(vJoint, 0.0);
                forceJoint = engineJointOptions_.boundStiffness * qJointError + damping;
            }

            float64_t blendingFactor = qJointError / engineJointOptions_.boundTransitionEps;
            float64_t blendingLaw = std::tanh(2 * blendingFactor);
            forceJoint *= blendingLaw;

            u(jointsVelocityIdx[i]) += forceJoint;
        }
    }

    configHolder_t Engine::getOptions(void) const
    {
        return engineOptionsHolder_;
    }

    void Engine::setOptions(configHolder_t const & engineOptions)
    {
        engineOptionsHolder_ = engineOptions;
        engineOptions_ = std::make_unique<engineOptions_t const>(engineOptionsHolder_);
        if (isInitialized_)
        {
            model_->pncModel_.gravity = engineOptions_->world.gravity; // It is reversed (Third Newton law)
        }
    }

    bool Engine::getIsInitialized(void) const
    {
        return isInitialized_;
    }

    Model const & Engine::getModel(void) const
    {
        return *model_;
    }

    void Engine::getLogData(std::vector<std::string> & header,
                            matrixN_t                & logData)
    {
        std::vector<float32_t> timestamps;
        std::vector<std::vector<int32_t> > intData;
        std::vector<std::vector<float32_t> > floatData;
        telemetryRecorder_->getData(header, timestamps, intData, floatData);

        // Never empty since it contains at least the initial state
        logData.resize(timestamps.size(), 1 + intData[0].size() + floatData[0].size());
        logData.col(0) = Eigen::Matrix<float32_t, 1, Eigen::Dynamic>::Map(timestamps.data(),
                                                                          timestamps.size()).cast<float64_t>();
        for (uint32_t i=0; i<intData.size(); i++)
        {
            logData.block(i, 1, 1, intData[i].size()) =
                Eigen::Matrix<int32_t, 1, Eigen::Dynamic>::Map(intData[i].data(),
                                                               intData[i].size()).cast<float64_t>();
        }
        for (uint32_t i=0; i<floatData.size(); i++)
        {
            logData.block(i, 1 + intData[0].size(), 1, floatData[i].size()) =
                Eigen::Matrix<float32_t, 1, Eigen::Dynamic>::Map(floatData[i].data(),
                                                                 floatData[i].size()).cast<float64_t>();
        }
    }

    void Engine::writeLogTxt(std::string const & filename)
    {
        std::vector<std::string> header;
        matrixN_t log;
        getLogData(header, log);

        std::ofstream myfile = std::ofstream(filename,
                                             std::ios::out |
                                             std::ofstream::trunc);

        auto indexConstantEnd = std::find(header.begin(), header.end(), START_COLUMNS);
        std::copy(header.begin()+1, indexConstantEnd-1, std::ostream_iterator<std::string>(myfile, ", ")); // Discard the first one (start constant flag)
        std::copy(indexConstantEnd-1, indexConstantEnd, std::ostream_iterator<std::string>(myfile, "\n"));
        std::copy(indexConstantEnd+1, header.end()-2, std::ostream_iterator<std::string>(myfile, ", "));
        std::copy(header.end()-2, header.end()-1, std::ostream_iterator<std::string>(myfile, "\n")); // Discard the last one (start data flag)

        Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");
        myfile << log.format(CSVFormat);

        myfile.close();
    }

    void Engine::writeLogBinary(std::string const & filename)
    {
        telemetryRecorder_->writeDataBinary(filename);
    }
}
