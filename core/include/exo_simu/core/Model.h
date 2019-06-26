#ifndef SIMU_MODEL_H
#define SIMU_MODEL_H

#include <string>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include "pinocchio/multibody/model.hpp"
#include "pinocchio/algorithm/frames.hpp"

#include "exo_simu/core/Types.h"


namespace exo_simu
{
    class AbstractSensor;

    class Model
    {
    public:
        virtual configHolder_t getDefaultJointOptions()
        {
            configHolder_t config;
            config["boundsFromUrdf"] = true; // Must be true since boundsMin and boundsMax are undefined
            config["boundsMin"] = vectorN_t();
            config["boundsMax"] = vectorN_t();

            return config;
        };

        struct jointOptions_t
        {
            bool      const boundsFromUrdf;
            vectorN_t const boundsMin;
            vectorN_t const boundsMax;

            jointOptions_t(configHolder_t const & options):
            boundsFromUrdf(boost::get<bool>(options.at("boundsFromUrdf"))),
            boundsMin(boost::get<vectorN_t>(options.at("boundsMin"))),
            boundsMax(boost::get<vectorN_t>(options.at("boundsMax")))
            {
                // Empty.
            }
        };

        virtual configHolder_t getDefaultOptions()
        {
            configHolder_t config;
            config["joints"] = getDefaultJointOptions();

            return config;
        };

        struct modelOptions_t
        {
            jointOptions_t const joints;

            modelOptions_t(configHolder_t const & options):
            joints(boost::get<configHolder_t>(options.at("joints")))
            {
                // Empty.
            }
        };

    public:
        Model(void);
        virtual ~Model(void);
        virtual Model* clone(void);

        result_t initialize(std::string              const & urdfPath, 
                            std::vector<std::string> const & contactFramesNames, 
                            std::vector<std::string> const & jointsNames);

        result_t addSensor(std::string    const & sensorType, 
                           AbstractSensor       * sensor);
        result_t removeSensor(std::string const & name);
        void removeSensors(void);

        configHolder_t getOptions(void) const;
        result_t setOptions(configHolder_t const & mdlOptions);
        bool getIsInitialized(void) const;
        std::string getUrdfPath(void) const;
        matrixN_t const & getSensorsData(std::string const & sensorType) const;
        void setSensorsData(float64_t const & t,
                            vectorN_t const & q,
                            vectorN_t const & v,
                            vectorN_t const & a,
                            vectorN_t const & u);
        std::vector<int32_t> const & getContactFramesIdx(void) const;
        std::vector<int32_t> const & getJointsPositionIdx(void) const;
        std::vector<int32_t> const & getJointsVelocityIdx(void) const;
        uint32_t nq(void) const; // no get keyword for consistency with pinocchio C++ API
        uint32_t nv(void) const;
        uint32_t nx(void) const;

    protected:
        result_t setUrdfPath(std::string const & urdfPath);
        result_t getFrameIdx(std::string const & frameName, 
                             int32_t           & frameIdx) const;
        result_t getFramesIdx(std::vector<std::string> const & framesNames, 
                              std::vector<int32_t>           & framesIdx) const;
        result_t getJointIdx(std::string const & jointName, 
                             int32_t           & jointPositionIdx, 
                             int32_t           & jointVelocityIdx) const;
        result_t getJointsIdx(std::vector<std::string> const & jointsNames, 
                              std::vector<int32_t>           & jointsPositionIdx, 
                              std::vector<int32_t>           & jointsVelocityIdx) const;
        
    public:
        pinocchio::Model pncModel_;
        pinocchio::Data pncData_;
        std::shared_ptr<modelOptions_t const> mdlOptions_;
        pinocchio::container::aligned_vector<pinocchio::Force> contactForces_; // Buffer to store the contact forces

    protected:
        bool isInitialized_;
        std::string urdfPath_;
        configHolder_t mdlOptionsHolder_;
        sensorsGroupHolder_t sensorsGroupHolder_;

        std::vector<std::string> contactFramesNames_;
        std::vector<std::string> jointsNames_;
        std::vector<int32_t> contactFramesIdx_;  // Indices of the contact frame in the model
        std::vector<int32_t> jointsPositionIdx_; // Indices of the actuated joints in the configuration representation
        std::vector<int32_t> jointsVelocityIdx_; // Indices of the actuated joints in the velocity vector representation

    private:
        uint32_t nq_;
        uint32_t nv_;
        uint32_t nx_;
    };
}

#endif //end of SIMU_MODEL_H