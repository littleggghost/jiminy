///////////////////////////////////////////////////////////////////////////////
/// \brief             Python exposition functions for Jiminy project.
////////////////////////////////////////////////////////////////////////////////

#ifndef SIMULATOR_PYTHON_H
#define SIMULATOR_PYTHON_H

#include <cassert>

#include "jiminy/core/Engine.h"
#include "jiminy/core/AbstractSensor.h"
#include "jiminy/core/Sensor.h"
#include "jiminy/core/Model.h"
#include "jiminy/core/AbstractController.h"
#include "jiminy/core/ControllerFunctor.h"
#include "jiminy/core/TelemetryData.h"
#include "jiminy/core/Types.h"
#include "jiminy/python/Utilities.h"

#include <boost/weak_ptr.hpp>
#include <boost/preprocessor.hpp>

#include <boost/python.hpp>
#include <boost/python/def.hpp>
#include <boost/python/dict.hpp>


namespace jiminy
{
namespace python
{
    namespace bp = boost::python;

    // ************************** TimeStateFctPyWrapper ******************************

    template<typename T>
    PyObject * getNumpyReference(T & data)
    {
        return getNumpyReferenceFromScalar(data);
    }

    template<>
    PyObject * getNumpyReference<vector3_t>(vector3_t & data)
    {
        return getNumpyReferenceFromEigenVector(data);
    }

    template<typename T>
    struct TimeStateFctPyWrapper {
    public:
        // Disable the copy of the class
        TimeStateFctPyWrapper & operator = (TimeStateFctPyWrapper const & other) = delete;

    public:
        TimeStateFctPyWrapper(bp::object const& objPy) :
        funcPyPtr_(objPy),
        outPtr_(new T),
        outPyPtr_()
        {
            outPyPtr_ = getNumpyReference(*outPtr_);
        }

        // Copy constructor, same as the normal constructor
        TimeStateFctPyWrapper(TimeStateFctPyWrapper const & other) :
        funcPyPtr_(other.funcPyPtr_),
        outPtr_(new T),
        outPyPtr_()
        {
            *outPtr_ = *(other.outPtr_);
            outPyPtr_ = getNumpyReference(*outPtr_);
        }

        // Move constructor, takes a rvalue reference &&
        TimeStateFctPyWrapper(TimeStateFctPyWrapper&& other) :
        funcPyPtr_(other.funcPyPtr_),
        outPtr_(nullptr),
        outPyPtr_(nullptr)
        {
            // Steal the resource from "other"
            outPtr_ = other.outPtr_;
            outPyPtr_ = other.outPyPtr_;

            /* "other" will soon be destroyed and its destructor will
               do nothing because we null out its resource here */
            other.outPtr_ = nullptr;
            other.outPyPtr_ = nullptr;
        }

        // Destructor
        ~TimeStateFctPyWrapper()
        {
            Py_XDECREF(outPyPtr_);
            delete outPtr_;
        }

        // Move assignment, takes a rvalue reference &&
        TimeStateFctPyWrapper& operator = (TimeStateFctPyWrapper&& other)
        {
            /* "other" is soon going to be destroyed, so we let it destroy our current resource
               instead and we take "other"'s current resource via swapping */
            std::swap(funcPyPtr_, other.funcPyPtr_);
            std::swap(outPtr_, other.outPtr_);
            std::swap(outPyPtr_, other.outPyPtr_);
            return *this;
        }

        T const & operator() (float64_t const & t,
                              vectorN_t const & x)
        {
            // Pass the arguments by reference (be careful const qualifiers are lost)
            bp::handle<> xPy(getNumpyReferenceFromEigenVector(x));
            bp::handle<> outPy(bp::borrowed(outPyPtr_));
            funcPyPtr_(t, xPy, outPy);
            return *outPtr_;
        }

    private:
        bp::object funcPyPtr_;
        T * outPtr_;
        PyObject * outPyPtr_;
    };

    enum class heatMapType_t : uint8_t
    {
        CONSTANT = 0x01,
        STAIRS   = 0x02,
        GENERIC  = 0x03,
    };

    struct HeatMapFunctorPyWrapper {
    public:
        // Disable the copy of the class
        HeatMapFunctorPyWrapper & operator = (HeatMapFunctorPyWrapper const & other) = delete;

    public:
        HeatMapFunctorPyWrapper(bp::object    const & objPy,
                                heatMapType_t const & objType) :
        heatMapType_(objType),
        handlePyPtr_(objPy),
        out1Ptr_(new float64_t),
        out2Ptr_(new vector3_t),
        out1PyPtr_(),
        out2PyPtr_()
        {
            if (heatMapType_ == heatMapType_t::CONSTANT)
            {
                *out1Ptr_ = bp::extract<float64_t>(handlePyPtr_);
                *out2Ptr_ = (vector3_t() << 0.0, 0.0, 1.0).finished();
            }
            else if (heatMapType_ == heatMapType_t::STAIRS)
            {
                out1PyPtr_ = getNumpyReference(*out1Ptr_);
                *out2Ptr_ = (vector3_t() << 0.0, 0.0, 1.0).finished();
            }
            else if (heatMapType_ == heatMapType_t::GENERIC)
            {
                out1PyPtr_ = getNumpyReference(*out1Ptr_);
                out2PyPtr_ = getNumpyReference(*out2Ptr_);
            }
        }

        // Copy constructor, same as the normal constructor
        HeatMapFunctorPyWrapper(HeatMapFunctorPyWrapper const & other) :
        heatMapType_(other.heatMapType_),
        handlePyPtr_(other.handlePyPtr_),
        out1Ptr_(new float64_t),
        out2Ptr_(new vector3_t),
        out1PyPtr_(),
        out2PyPtr_()
        {
            *out1Ptr_ = *(other.out1Ptr_);
            *out2Ptr_ = *(other.out2Ptr_);
            out1PyPtr_ = getNumpyReference(*out1Ptr_);
            out2PyPtr_ = getNumpyReference(*out2Ptr_);
        }

        // Move constructor, takes a rvalue reference &&
        HeatMapFunctorPyWrapper(HeatMapFunctorPyWrapper && other) :
        heatMapType_(other.heatMapType_),
        handlePyPtr_(other.handlePyPtr_),
        out1Ptr_(nullptr),
        out2Ptr_(nullptr),
        out1PyPtr_(nullptr),
        out2PyPtr_(nullptr)
        {
            // Steal the resource from "other"
            out1Ptr_ = other.out1Ptr_;
            out2Ptr_ = other.out2Ptr_;
            out1PyPtr_ = other.out1PyPtr_;
            out2PyPtr_ = other.out2PyPtr_;

            /* "other" will soon be destroyed and its destructor will
               do nothing because we null out its resource here */
            other.out1Ptr_ = nullptr;
            other.out2Ptr_ = nullptr;
            other.out1PyPtr_ = nullptr;
            other.out2PyPtr_ = nullptr;
        }

        // Destructor
        ~HeatMapFunctorPyWrapper()
        {
            Py_XDECREF(out1PyPtr_);
            Py_XDECREF(out2PyPtr_);
            delete out1Ptr_;
            delete out2Ptr_;
        }

        // Move assignment, takes a rvalue reference &&
        HeatMapFunctorPyWrapper& operator = (HeatMapFunctorPyWrapper&& other)
        {
            /* "other" is soon going to be destroyed, so we let it destroy our current resource
               instead and we take "other"'s current resource via swapping */
            std::swap(heatMapType_, other.heatMapType_);
            std::swap(handlePyPtr_, other.handlePyPtr_);
            std::swap(out1Ptr_, other.out1Ptr_);
            std::swap(out2Ptr_, other.out2Ptr_);
            std::swap(out1PyPtr_, other.out1PyPtr_);
            std::swap(out2PyPtr_, other.out2PyPtr_);
            return *this;
        }

        std::pair<float64_t, vector3_t> operator() (vector3_t const & posFrame)
        {
            // Pass the arguments by reference (be careful const qualifiers are lost)

            if (heatMapType_ == heatMapType_t::STAIRS)
            {
                bp::handle<> out1Py(bp::borrowed(out1PyPtr_));
                handlePyPtr_(posFrame[0], posFrame[1], out1Py);
            }
            else if (heatMapType_ == heatMapType_t::GENERIC)
            {
                bp::handle<> out1Py(bp::borrowed(out1PyPtr_));
                bp::handle<> out2Py(bp::borrowed(out2PyPtr_));
                handlePyPtr_(posFrame[0], posFrame[1], out1Py, out2Py);
            }

            return {*out1Ptr_, *out2Ptr_};
        }

    private:
        heatMapType_t heatMapType_;
        bp::object handlePyPtr_;
        float64_t * out1Ptr_;
        vector3_t * out2Ptr_;
        PyObject * out1PyPtr_;
        PyObject * out2PyPtr_;
    };


    // **************************** HeatMapFunctorVisitor *****************************

    struct HeatMapFunctorVisitor
        : public bp::def_visitor<HeatMapFunctorVisitor>
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            cl
                .def("__call__", &HeatMapFunctorVisitor::eval,
                                 (bp::arg("self"), bp::arg("position")))
                ;
        }

        static bp::tuple eval (heatMapFunctor_t       & self,
                               vector3_t        const & posFrame)
        {
            std::pair<float64_t, vector3_t> ground = self(posFrame);
            return bp::make_tuple(std::move(std::get<0>(ground)), std::move(std::get<1>(ground)));
        }

        static boost::shared_ptr<heatMapFunctor_t> HeatMapFunctorPyFactory(bp::object          & objPy,
                                                                           heatMapType_t const & objType)
        {
            return boost::make_shared<heatMapFunctor_t>(HeatMapFunctorPyWrapper(std::move(objPy), objType));
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<heatMapFunctor_t,
                       boost::shared_ptr<heatMapFunctor_t> >("HeatMapFunctor", bp::no_init)
                .def("__init__", bp::make_constructor(&HeatMapFunctorVisitor::HeatMapFunctorPyFactory,
                                 bp::default_call_policies(),
                                (bp::args("heatmap_handle", "heatmap_type"))))
                .def(HeatMapFunctorVisitor());
        }
    };

    // ******************************* sensorsDataMap_t ********************************


    struct SensorsDataMapVisitor
        : public bp::def_visitor<SensorsDataMapVisitor>
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            cl
                .def("__len__", &SensorsDataMapVisitor::len,
                                (bp::arg("self")))
                .def("__getitem__", &SensorsDataMapVisitor::getItem,
                                    (bp::arg("self"), "(sensor_type, sensor_name)"))
                .def("__getitem__", &SensorsDataMapVisitor::getItemSplit,
                                    (bp::arg("self"), "sensor_type", "sensor_name"))
                .def("__getitem__", &SensorsDataMapVisitor::getSub,
                                    (bp::arg("self"), "sensor_type"))
                .def("__iter__", bp::iterator<sensorsDataMap_t>())
                .def("__contains__", &SensorsDataMapVisitor::contains,
                                     (bp::arg("self"), "key"))
                .def("keys", &SensorsDataMapVisitor::keys,
                             (bp::arg("self")))
                .def("keys", &SensorsDataMapVisitor::keysSensorType,
                             (bp::arg("self"), "sensor_type"))
                .def("values", &SensorsDataMapVisitor::values,
                               (bp::arg("self")))
                .def("items", &SensorsDataMapVisitor::items,
                              (bp::arg("self")))
                ;
        }

        static uint32_t len(sensorsDataMap_t & self)
        {
            return self.size();
        }

        static bp::object getItem(sensorsDataMap_t        & self,
                                  bp::tuple         const & sensorInfo)
        {
            std::string sensorType = bp::extract<std::string>(sensorInfo[0]);
            std::string sensorName = bp::extract<std::string>(sensorInfo[1]);
            return SensorsDataMapVisitor::getItemSplit(self, sensorType, sensorName);
        }

        static bp::object getItemSplit(sensorsDataMap_t       & self,
                                       std::string      const & sensorType,
                                       std::string      const & sensorName)
        {
            try
            {
                auto & sensorDataTypeByName = self.at(sensorType).get<IndexByName>();
                auto sensorDataIt = sensorDataTypeByName.find(sensorName);
                vectorN_t const * sensorDataValue = sensorDataIt->value;
                bp::handle<> valuePy(getNumpyReferenceFromEigenVector(*sensorDataValue));
                return bp::object(valuePy);
            }
            catch (...)
            {
                PyErr_SetString(PyExc_KeyError, "The key does not exist.");
                return bp::object();
            }
        }

        static matrixN_t getSub(sensorsDataMap_t       & self,
                                std::string      const & sensorType)
        {
            // Extract the encoder data matrix
            auto const & sensorsDataType = self.at(sensorType);
            matrixN_t data;
            auto sensorDataIt = sensorsDataType.begin();
            data.resize(sensorDataIt->value->size(), sensorsDataType.size());
            data.col(sensorDataIt->id) = *sensorDataIt->value;
            ++sensorDataIt;
            for (; sensorDataIt != sensorsDataType.end(); ++sensorDataIt)
            {
                data.col(sensorDataIt->id) = *sensorDataIt->value;
            }
            return data;
        }

        static bool contains(sensorsDataMap_t       & self,
                             bp::tuple        const & sensorInfo)
        {
            std::string sensorType = bp::extract<std::string>(sensorInfo[0]);
            std::string sensorName = bp::extract<std::string>(sensorInfo[1]);
            auto const & sensorDataType = self.find(sensorType);
            if (sensorDataType != self.end())
            {
                auto & sensorDataTypeByName = sensorDataType->second.get<IndexByName>();
                auto sensorDataIt = sensorDataTypeByName.find(sensorName);
                if (sensorDataIt != sensorDataTypeByName.end())
                {
                    return true;
                }
            }
            return false;
        }

        static bp::list keys(sensorsDataMap_t & self)
        {
            bp::list sensorsInfo;
            for (auto & sensorData : self)
            {
                sensorsInfo.append(sensorData.first);
            }
            return sensorsInfo;
        }

        static bp::list keysSensorType(sensorsDataMap_t & self,
                                       std::string const& sensorType)
        {
            bp::list sensorsInfo;
            for (auto & sensorData : self.at(sensorType))
            {
                sensorsInfo.append(sensorData.name);
            }
            return sensorsInfo;
        }

        static bp::list values(sensorsDataMap_t & self)
        {
            bp::list sensorsValue;
            for (auto const & sensorDataType : self)
            {
                sensorsValue.append(getSub(self, sensorDataType.first));
            }
            return sensorsValue;
        }

        static bp::list items(sensorsDataMap_t & self)
        {
            bp::list sensorsDataPy;
            for (auto const & sensorDataType : self)
            {
                sensorsDataPy.append(bp::make_tuple(sensorDataType.first,
                                                    getSub(self, sensorDataType.first)));
            }
            return sensorsDataPy;
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<sensorsDataMap_t,
                       boost::shared_ptr<sensorsDataMap_t>,
                       boost::noncopyable>("sensorsData", bp::no_init)
                .def(SensorsDataMapVisitor());
        }
    };

    // **************************** ControllerFctWrapper *******************************

    struct ControllerFctWrapper
    {
    public:
        ControllerFctWrapper(bp::object const & objPy) : funcPyPtr_(objPy) {}
        void operator() (float64_t        const & t,
                         vectorN_t        const & q,
                         vectorN_t        const & v,
                         sensorsDataMap_t const & sensorsData,
                         vectorN_t              & uCommand)
        {
            // Pass the arguments by reference (be careful const qualifiers are lost).
            bp::handle<> qPy(getNumpyReferenceFromEigenVector(q));
            bp::handle<> vPy(getNumpyReferenceFromEigenVector(v));
            bp::handle<> uCommandPy(getNumpyReferenceFromEigenVector(uCommand));
            funcPyPtr_(t, qPy, vPy, boost::ref(sensorsData), uCommandPy);
        }
    private:
        bp::object funcPyPtr_;
    };

    // ***************************** PySensorVisitor ***********************************

    struct PySensorVisitor
        : public bp::def_visitor<PySensorVisitor>
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////

        template<class PyClass>
        class PySensorVisit
        {
        public:
            using TSensor = typename PyClass::wrapped_type;

            static void visitAbstract(PyClass& cl)
            {
                cl
                    .def("get_options", &PySensorVisitor::getOptions<TSensor>,
                                        bp::return_value_policy<bp::return_by_value>())
                    .def("set_options", &PySensorVisitor::setOptions<TSensor>)

                    .add_property("name", bp::make_function(&AbstractSensorBase::getName,
                                          bp::return_value_policy<bp::copy_const_reference>()))
                    .add_property("is_initialized", bp::make_function(&AbstractSensorBase::getIsInitialized,
                                                    bp::return_value_policy<bp::copy_const_reference>()))
                    ;
            }

            template<class Q = TSensor>
            static typename std::enable_if<!std::is_same<Q, AbstractSensorBase>::value, void>::type
            visit(PyClass& cl)
            {
                visitAbstract(cl);

                cl
                    .def("initialize", &TSensor::initialize)
                    .def_readonly("type", &TSensor::type_)
                    .add_static_property("fieldnames", bp::make_function(&PySensorVisitor::getFieldNamesStatic<TSensor>,
                                                       bp::return_value_policy<bp::return_by_value>()))
                    ;
            }

            template<class Q = TSensor>
            static typename std::enable_if<std::is_same<Q, AbstractSensorBase>::value, void>::type
            visit(PyClass& cl)
            {
                visitAbstract(cl);

                cl
                    .add_property("type", bp::make_function(&AbstractSensorBase::getType,
                                          bp::return_value_policy<bp::copy_const_reference>()))
                    .add_property("fieldnames", bp::make_function(&PySensorVisitor::getFieldNames,
                                                bp::return_value_policy<bp::return_by_value>()))
                    ;
            }
        };

    public:
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            PySensorVisit<PyClass>::visit(cl);
        }

        template<typename TSensor>
        static std::shared_ptr<TSensor> pySensorFactory(void)
        {
            return {nullptr};
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief      Getters and Setters
        ///////////////////////////////////////////////////////////////////////////////

        template<typename TSensor>
        static bp::dict getOptions(TSensor & self)
        {
            bp::dict configPy;
            convertToPy(self.getOptions(), configPy);
            return configPy;
        }

        template<typename TSensor>
        static void setOptions(TSensor        & self,
                               bp::dict const & configPy)
        {
            configHolder_t config = self.getOptions();
            convertToC(configPy, config);
            self.setOptions(config);
        }

        template<typename TSensor>
        static bp::list getFieldNamesStatic(void)
        {
            return stdVectorToListPy(TSensor::fieldNames_);
        }

        static bp::list getFieldNames(AbstractSensorBase & self)
        {
            return stdVectorToListPy(self.getFieldNames());
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<AbstractSensorBase,
                       boost::shared_ptr<AbstractSensorBase>,
                       boost::noncopyable>("AbstractSensor", bp::no_init)
                .def(PySensorVisitor());
            bp::register_ptr_to_python<std::shared_ptr<AbstractSensorBase> >(); // Required to handle std::shared_ptr from/to Python (as opposed to boost::shared_ptr)

            bp::class_<ImuSensor, bp::bases<AbstractSensorBase>,
                       boost::shared_ptr<ImuSensor>,
                       boost::noncopyable>("ImuSensor", bp::no_init)
                .def(PySensorVisitor());
            bp::register_ptr_to_python<std::shared_ptr<ImuSensor> >();

            bp::class_<ForceSensor, bp::bases<AbstractSensorBase>,
                       boost::shared_ptr<ForceSensor>,
                       boost::noncopyable>("ForceSensor", bp::no_init)
                .def(PySensorVisitor());
            bp::register_ptr_to_python<std::shared_ptr<ForceSensor> >();

            bp::class_<EncoderSensor, bp::bases<AbstractSensorBase>,
                       boost::shared_ptr<EncoderSensor>,
                       boost::noncopyable>("EncoderSensor", bp::no_init)
                .def(PySensorVisitor());
            bp::register_ptr_to_python<std::shared_ptr<EncoderSensor> >();
        }
    };

    // ***************************** PyModelVisitor ***********************************

    struct PyModelVisitor
        : public bp::def_visitor<PyModelVisitor>
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            cl
                .def("initialize", &PyModelVisitor::initialize,
                                   (bp::arg("self"), "urdf_path",
                                    bp::arg("contacts") = std::vector<std::string>(),
                                    bp::arg("motors") = std::vector<std::string>(),
                                    bp::arg("has_freeflyer") = false))

                .def("add_imu_sensor", &PyModelVisitor::createAndAddSensor<ImuSensor>,
                                       (bp::arg("self"),
                                        bp::arg("sensor_name") = std::string(),
                                        "frame_name"))
                .def("add_force_sensor", &PyModelVisitor::createAndAddSensor<ForceSensor>,
                                         (bp::arg("self"),
                                          bp::arg("sensor_name") = std::string(),
                                          "frame_name"))
                .def("add_encoder_sensor", &PyModelVisitor::createAndAddSensor<EncoderSensor>,
                                           (bp::arg("self"),
                                            bp::arg("sensor_name") = std::string(),
                                            "joint_name"))
                .def("remove_sensor", &Model::removeSensor,
                                      (bp::arg("self"), "sensor_type", "sensor_name"))
                .def("remove_sensors", &Model::removeSensors,
                                       (bp::arg("self"), "sensorType"))
                .def("get_sensor", &PyModelVisitor::getSensor,
                                   (bp::arg("self"), "sensor_type", "sensor_name"),
                                   bp::return_value_policy<bp::reference_existing_object>())

                .def("get_model_options", &PyModelVisitor::getModelOptions,
                                          bp::return_value_policy<bp::return_by_value>())
                .def("set_model_options", &PyModelVisitor::setModelOptions)
                .def("get_sensors_options", &PyModelVisitor::getSensorsOptions,
                                            bp::return_value_policy<bp::return_by_value>())
                .def("set_sensors_options", &PyModelVisitor::setSensorsOptions)

                .add_property("sensors_data", bp::make_function(&PyModelVisitor::getSensorsData,
                                              bp::return_value_policy<bp::manage_new_object>()))

                .add_property("frames_names", &PyModelVisitor::getFramesNames)

                .add_property("pinocchio_model", bp::make_getter(&Model::pncModel_,
                                                 bp::return_internal_reference<>()))
                .add_property("pinocchio_data", bp::make_getter(&Model::pncData_,
                                                bp::return_internal_reference<>()))
                .add_property("pinocchio_model_th", bp::make_getter(&Model::pncModelRigidOrig_,
                                                 bp::return_internal_reference<>()))
                .add_property("pinocchio_data_th", bp::make_getter(&Model::pncDataRigidOrig_,
                                                bp::return_internal_reference<>()))

                .add_property("is_initialized", bp::make_function(&Model::getIsInitialized,
                                                bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("urdf_path", bp::make_function(&Model::getUrdfPath,
                                           bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("has_freeflyer", bp::make_function(&Model::getHasFreeflyer,
                                               bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("is_flexible", &PyModelVisitor::isFlexibleModelEnable)
                .add_property("motors_names", bp::make_function(&Model::getMotorsNames,
                                              bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("rigid_joints_names", bp::make_function(&Model::getRigidJointsNames,
                                                    bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("flexible_joints_names", bp::make_function(&Model::getFlexibleJointsNames,
                                                       bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("contact_frames_idx", bp::make_function(&Model::getContactFramesIdx,
                                                    bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("motors_position_idx", bp::make_function(&Model::getMotorsPositionIdx,
                                                     bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("motors_velocity_idx", bp::make_function(&Model::getMotorsVelocityIdx,
                                                     bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("rigid_joints_position_idx", bp::make_function(&Model::getRigidJointsPositionIdx,
                                                           bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("rigid_joints_velocity_idx", bp::make_function(&Model::getRigidJointsVelocityIdx,
                                                           bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("position_fieldnames", bp::make_function(&Model::getPositionFieldNames,
                                                     bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("position_limit_upper", bp::make_function(&Model::getPositionLimitMin,
                                                      bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("position_limit_lower", bp::make_function(&Model::getPositionLimitMax,
                                                      bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("velocity_fieldnames", bp::make_function(&Model::getVelocityFieldNames,
                                                     bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("velocity_limit", bp::make_function(&Model::getVelocityLimit,
                                                bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("acceleration_fieldnames", bp::make_function(&Model::getAccelerationFieldNames,
                                                         bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("motor_torque_fieldnames", bp::make_function(&Model::getMotorTorqueFieldNames,
                                                         bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("nq", bp::make_function(&Model::nq,
                                    bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("nv", bp::make_function(&Model::nv,
                                    bp::return_value_policy<bp::copy_const_reference>()))
                .add_property("nx", bp::make_function(&Model::nx,
                                    bp::return_value_policy<bp::copy_const_reference>()))
                ;
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief      Initialize the model
        ///////////////////////////////////////////////////////////////////////////////
        static result_t initialize(Model             & self,
                                   std::string const & urdfPath,
                                   bp::list    const & contactFramesNamesPy,
                                   bp::list    const & motorsNamesPy,
                                   bool        const & hadFreeflyer)
        {
            std::vector<std::string> contactFramesNames = listPyToStdVector<std::string>(contactFramesNamesPy);
            std::vector<std::string> motorsNames = listPyToStdVector<std::string>(motorsNamesPy);
            return self.initialize(urdfPath, contactFramesNames, motorsNames, hadFreeflyer);
        }

        template<typename TSensor>
        static result_t createAndAddSensor(Model             & self,
                                           std::string         sensorName,
                                           std::string const & name)
        {
            result_t returnCode = result_t::SUCCESS;

            if (sensorName.empty())
            {
                sensorName = name;
            }

            std::shared_ptr<TSensor> sensor;
            returnCode = self.addSensor(sensorName, sensor);

            if (returnCode == result_t::SUCCESS)
            {
                returnCode = sensor-> initialize(name);
            }

            return returnCode;
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief      Getters and Setters
        ///////////////////////////////////////////////////////////////////////////////

        static sensorsDataMap_t * getSensorsData(Model & model)
        {
            sensorsDataMap_t * data = new sensorsDataMap_t();
            model.getSensorsData(*data);
            return data;
        }

        static AbstractSensorBase * getSensor(Model             & self,
                                              std::string const & sensorType,
                                              std::string const & sensorName)
        {
            std::shared_ptr<AbstractSensorBase> sensor;
            self.getSensor(sensorType, sensorName, sensor);
            return sensor.get();
        }

        static std::vector<std::string> getFramesNames(Model & self)
        {
            pinocchio::container::aligned_vector<pinocchio::Frame> frames =
                self.pncModel_.frames;
            std::vector<std::string> framesNames;
            for (pinocchio::Frame const & frame : frames)
            {
                framesNames.push_back(frame.name);
            }
            return framesNames;
        }

        static bool isFlexibleModelEnable(Model & self)
        {
            return self.mdlOptions_->dynamics.enableFlexibleModel;
        }

        static std::vector<std::string> getFlexibleOnlyJointsNames(Model & self)
        {
            flexibilityConfig_t const & flexibilityConfig = self.mdlOptions_->dynamics.flexibilityConfig;
            std::vector<std::string> flexibleJointNames;
            for (flexibleJointData_t const & flexibleJoint : flexibilityConfig)
            {
                flexibleJointNames.emplace_back(flexibleJoint.jointName);
            }
            return flexibleJointNames;
        }

        static bp::dict getModelOptions(Model & self)
        {
            bp::dict configModelPy;
            convertToPy(self.getOptions(), configModelPy);

            bp::dict configTelemetryPy;
            configHolder_t configTelemetry;
            self.getTelemetryOptions(configTelemetry);
            convertToPy(configTelemetry, configTelemetryPy);
            configModelPy["telemetry"] = configTelemetryPy;

            return configModelPy;
        }

        static void setModelOptions(Model          & self,
                                    bp::dict const & configPy)
        {
            configHolder_t configModel = self.getOptions();
            convertToC(configPy, configModel);
            self.setOptions(configModel);

            configHolder_t configTelemetry;
            self.getTelemetryOptions(configTelemetry);
            convertToC(bp::extract<bp::dict>(configPy["telemetry"]), configTelemetry);
            self.setTelemetryOptions(configTelemetry);
        }

        static bp::dict getSensorsOptions(Model & self)
        {
            configHolder_t config;
            bp::dict configPy;
            self.getSensorsOptions(config);
            convertToPy(config, configPy);

            return configPy;
        }

        static void setSensorsOptions(Model          & self,
                                      bp::dict const & configPy)
        {
            configHolder_t config;
            self.getSensorsOptions(config);
            convertToC(configPy, config);
            self.setSensorsOptions(config);
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<Model,
                       boost::shared_ptr<Model>,
                       boost::noncopyable>("Model")
                .def(PyModelVisitor());
            bp::register_ptr_to_python<std::shared_ptr<Model> >();
        }
    };

    // ***************************** PyAbstractControllerVisitor ***********************************

    struct PyAbstractControllerVisitor
        : public bp::def_visitor<PyAbstractControllerVisitor>
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            cl
                .def("initialize", &AbstractController::initialize,
                                   (bp::arg("self"), "model"))
                .def("register_entry", &PyAbstractControllerVisitor::registerNewEntry,
                                       (bp::arg("self"), "fieldname", "value"),
                                       "@copydoc AbstractController::registerNewEntry")
                .def("register_entry", &PyAbstractControllerVisitor::registerNewVectorEntry,
                                       (bp::arg("self"), "fieldnames", "values"))
                .def("remove_entries", &AbstractController::removeEntries)
                .def("get_options", &PyAbstractControllerVisitor::getOptions,
                                    bp::return_value_policy<bp::return_by_value>())
                .def("set_options", &PyAbstractControllerVisitor::setOptions)
                ;
        }

        static result_t registerNewEntry(AbstractController       & self,
                                         std::string        const & fieldName,
                                         PyObject                 * dataPy)
        {
            // Note that  Const qualifier is not supported by PyArray_DATA

            const char* p = Py_TYPE(dataPy)->tp_name;
            if (p == std::string("numpy.ndarray"))
            {
                float64_t const * data = (float64_t *) PyArray_DATA(reinterpret_cast<PyArrayObject *>(dataPy));
                return self.registerNewEntry(fieldName, *data);
            }
            else
            {
                std::cout << "Error - PyAbstractControllerVisitor::registerNewEntry - 'value' input must have type 'numpy.ndarray'." << std::endl;
                return result_t::ERROR_BAD_INPUT;
            }
        }

        static result_t registerNewVectorEntry(AbstractController       & self,
                                               bp::list           const & fieldNamesPy,
                                               PyObject                 * dataPy)
        {
            // Note that  Const qualifier is not supported by PyArray_DATA

            const char* p = Py_TYPE(dataPy)->tp_name;
            if (p == std::string("numpy.ndarray"))
            {
                std::vector<std::string> fieldNames = listPyToStdVector<std::string>(fieldNamesPy);
                Eigen::Map<vectorN_t> data((float64_t *) PyArray_DATA(reinterpret_cast<PyArrayObject *>(dataPy)), fieldNames.size());
                return self.registerNewVectorEntry(fieldNames, data);
            }
            else
            {
                std::cout << "Error - PyAbstractControllerVisitor::registerNewVectorEntry - 'values' input must have type 'numpy.ndarray'." << std::endl;
                return result_t::ERROR_BAD_INPUT;
            }
        }

        static bp::dict getOptions(AbstractController & self)
        {
            bp::dict configPy;
            convertToPy(self.getOptions(), configPy);
            return configPy;
        }

        static void setOptions(AbstractController       & self,
                               bp::dict           const & configPy)
        {
            configHolder_t config = self.getOptions();
            convertToC(configPy, config);
            self.setOptions(config);
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<AbstractController,
                       boost::shared_ptr<AbstractController>,
                       boost::noncopyable>("AbstractController", bp::no_init)
                .def(PyAbstractControllerVisitor());
        }
    };

    // ***************************** PyControllerFunctorVisitor ***********************************

    struct PyControllerFunctorVisitor
        : public bp::def_visitor<PyControllerFunctorVisitor>
    {
    public:
        typedef ControllerFunctor<ControllerFctWrapper, ControllerFctWrapper> CtrlFunctor;

    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            cl
                .def("initialize", &PyControllerFunctorVisitor::initialize,
                                   (bp::arg("self"), "model"))
                ;
        }

        static boost::shared_ptr<CtrlFunctor> ControllerFunctorPyFactory(bp::object & commandPy,
                                                                         bp::object & internalDynamicsPy)
        {
            ControllerFctWrapper commandFct(commandPy);
            ControllerFctWrapper internalDynamicsFct(internalDynamicsPy);
            return boost::make_shared<CtrlFunctor>(std::move(commandFct),
                                                   std::move(internalDynamicsFct));
        }

        static void initialize(CtrlFunctor                  & self,
                               std::shared_ptr<Model> const & model)
        {
            // Cannot pass const shared_ptr from Python to C++ directly...

            self.initialize(model);
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<CtrlFunctor, bp::bases<AbstractController>,
                       boost::shared_ptr<CtrlFunctor>,
                       boost::noncopyable>("ControllerFunctor", bp::no_init)
            .def(PyControllerFunctorVisitor())
            .def("__init__", bp::make_constructor(&PyControllerFunctorVisitor::ControllerFunctorPyFactory,
                             bp::default_call_policies(),
                            (bp::arg("command_handle"), "internal_dynamics_handle")));
        }
    };

    // ***************************** PyStepperVisitor ***********************************

    struct PyStepperVisitor
        : public bp::def_visitor<PyStepperVisitor>
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            cl
                .def_readonly("iter", &stepperState_t::iter)
                .def_readonly("t", &stepperState_t::t)
                .def_readonly("dt", &stepperState_t::dt)
                .add_property("x", bp::make_getter(&stepperState_t::x,
                                   bp::return_value_policy<bp::copy_non_const_reference>()))
                .add_property("q", bp::make_function(&PyStepperVisitor::q,
                                   bp::return_value_policy<bp::return_by_value>()))
                .add_property("v", bp::make_function(&PyStepperVisitor::v,
                                   bp::return_value_policy<bp::return_by_value>()))
                .add_property("dxdt", bp::make_getter(&stepperState_t::dxdt,
                                      bp::return_value_policy<bp::copy_non_const_reference>()))
                .add_property("qDot", bp::make_function(&PyStepperVisitor::qDot,
                                      bp::return_value_policy<bp::return_by_value>()))
                .add_property("a", bp::make_function(&PyStepperVisitor::a,
                                   bp::return_value_policy<bp::return_by_value>()))
                .add_property("u", bp::make_getter(&stepperState_t::u,
                                   bp::return_value_policy<bp::copy_non_const_reference>()))
                .add_property("u_command", bp::make_getter(&stepperState_t::uCommand,
                                           bp::return_value_policy<bp::copy_non_const_reference>()))
                .add_property("u_internal", bp::make_getter(&stepperState_t::uInternal,
                                            bp::return_value_policy<bp::copy_non_const_reference>()))
                .add_property("f_external", bp::make_getter(&stepperState_t::fExternal,
                                            bp::return_value_policy<bp::copy_non_const_reference>()))
                .def_readonly("energy", &stepperState_t::energy)
                ;
        }

        static vectorN_t q(stepperState_t & self)
        {
            // Eigenpy is not able to convert automatically a Eigen::Ref object

            return self.q();
        }

        static vectorN_t v(stepperState_t & self)
        {
            // Eigenpy is not able to convert automatically a Eigen::Ref object

            return self.v();
        }

        static vectorN_t qDot(stepperState_t & self)
        {
            // Eigenpy is not able to convert automatically a Eigen::Ref object

            return self.qDot();
        }

        static vectorN_t a(stepperState_t & self)
        {
            // Eigenpy is not able to convert automatically a Eigen::Ref object

            return self.a();
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<stepperState_t,
                       boost::shared_ptr<stepperState_t>,
                       boost::noncopyable>("StepperState", bp::no_init)
                .def(PyStepperVisitor());
        }
    };

    // ***************************** PyEngineVisitor ***********************************

    struct PyEngineVisitor
        : public bp::def_visitor<PyEngineVisitor>
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose C++ API through the visitor.
        ///////////////////////////////////////////////////////////////////////////////
        template<class PyClass>
        void visit(PyClass& cl) const
        {
            cl
                .def("initialize", &PyEngineVisitor::initialize,
                                   (bp::arg("self"), "model", "controller"))
                .def("initialize", &PyEngineVisitor::initializeWithCallback,
                                   (bp::arg("self"), "model", "controller", "callback_handle"))
                .def("simulate", &Engine::simulate,
                                 (bp::arg("self"), "x_init", "end_time"))
                .def("step", &PyEngineVisitor::step,
                             (bp::arg("self"), bp::arg("dt_desired")=-1))
                .def("reset", &Engine::reset)
                .def("set_state", &PyEngineVisitor::setState,
                              (bp::arg("self"), "x_init", bp::arg("reset_random_generator")=false))

                .def("get_log", &PyEngineVisitor::getLog)
                .def("write_log", &PyEngineVisitor::writeLog,
                                  (bp::arg("self"), "filename", bp::arg("isModeBinary")=true))
                .def("read_log", &PyEngineVisitor::parseLogBinary, (bp::arg("filename")))
                .staticmethod("read_log")

                .def("register_force_impulse", &Engine::registerForceImpulse,
                                               (bp::arg("self"), "frame_name", "t", "dt", "F"))
                .def("register_force_profile", &PyEngineVisitor::registerForceProfile,
                                               (bp::arg("self"), "frame_name", "force_handle"))
                .def("remove_forces", &PyEngineVisitor::removeForces)

                .def("get_options", &PyEngineVisitor::getOptions,
                                    bp::return_value_policy<bp::return_by_value>())
                .def("set_options", &PyEngineVisitor::setOptions)

                .add_property("stepper_state", bp::make_function(&Engine::getStepperState,
                                               bp::return_internal_reference<>()))
                .add_property("model", bp::make_function(&Engine::getModel,
                                       bp::return_internal_reference<>()))
                .add_property("controller", bp::make_function(&Engine::getController,
                                            bp::return_internal_reference<>()))
                ;
        }

        static result_t initialize(Engine                                    & self,
                                   std::shared_ptr<Model>              const & model,
                                   std::shared_ptr<AbstractController> const & controller)
        {
            Engine::callbackFunctor_t callbackFct = [](float64_t const & t,
                                                       vectorN_t const & x) -> bool
                                                    {
                                                        return true;
                                                    };
            return self.initialize(model, controller, std::move(callbackFct));
        }

        static result_t initializeWithCallback(Engine                                    & self,
                                               std::shared_ptr<Model>              const & model,
                                               std::shared_ptr<AbstractController> const & controller,
                                               bp::object                          const & callbackPy)
        {
            TimeStateFctPyWrapper<bool> callbackFct(callbackPy);
            return self.initialize(model, controller, std::move(callbackFct));
        }

        static result_t setState(Engine          & self,
                                 vectorN_t const & x_init,
                                 bool      const & resetRandomNumbers)
        {
            // Only way to handle C++ default values that are not accessible in Python
            return self.setState(x_init, resetRandomNumbers);
        }

        static result_t step(Engine          & self,
                             float64_t const & dtDesired)
        {
            // Only way to handle C++ default values that are not accessible in Python
            return self.step(dtDesired);
        }

        static void writeLog(Engine            & self,
                             std::string const & filename,
                             bool        const & isModeBinary)
        {
            if (isModeBinary)
            {
                self.writeLogBinary(filename);
            }
            else
            {
                self.writeLogTxt(filename);
            }
        }

        static void registerForceProfile(Engine            & self,
                                         std::string const & frameName,
                                         bp::object  const & forcePy)
        {
            TimeStateFctPyWrapper<vector3_t> forceFct(forcePy);
            self.registerForceProfile(frameName, std::move(forceFct));
        }

        static void removeForces(Engine & self)
        {
            self.reset(true);
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief      Getters and Setters
        ///////////////////////////////////////////////////////////////////////////////

        static bp::tuple formatLog(std::vector<std::string>             const & header,
                                   std::vector<float32_t>               const & timestamps,
                                   std::vector<std::vector<int32_t> >         & intData,
                                   std::vector<std::vector<float32_t> >       & floatData,
                                   bool                                 const & clear_memory = true)
        {
            bp::dict constants;
            bp::dict data;

            // Get constants
            uint32_t lastConstantId = std::distance(header.begin(), std::find(header.begin(), header.end(), START_COLUMNS));
            for (uint32_t i = 1; i < lastConstantId; i++)
            {
                int32_t delimiter = header[i].find("=");
                constants[header[i].substr(0, delimiter)] = header[i].substr(delimiter + 1);
            }

            // Get Global.Time
            Eigen::Ref<Eigen::Matrix<float32_t, Eigen::Dynamic, 1> const> timeBuffer =
                Eigen::Matrix<float32_t, Eigen::Dynamic, 1>::Map(
                    timestamps.data(), timestamps.size());
            PyObject * valuePyTime(getNumpyReferenceFromEigenVector(timeBuffer));
            data[header[lastConstantId + 1]] = bp::object(bp::handle<>(PyArray_FROM_OF(valuePyTime, NPY_ARRAY_ENSURECOPY)));
            Py_XDECREF(valuePyTime);

            // Get intergers
            Eigen::Matrix<int32_t, Eigen::Dynamic, Eigen::Dynamic> intDataMatrix;
            intDataMatrix.resize(timestamps.size(), intData[0].size());
            for (uint32_t i=0; i<intData.size(); i++)
            {
                intDataMatrix.row(i) = Eigen::Matrix<int32_t, 1, Eigen::Dynamic>::Map(
                    intData[i].data(), intData[0].size());
            }
            if (clear_memory)
            {
                intData.clear();
            }

            for (uint32_t i=0; i<intData[0].size(); i++)
            {
                PyObject * valuePyInt(getNumpyReferenceFromEigenVector(intDataMatrix.col(i)));
                std::string const & header_i = header[i + (lastConstantId + 1) + 1];
                // One must make copies with PyArray_FROM_OF instead of using raw pointer for floatDataMatrix
                // and setting NPY_ARRAY_OWNDATA because otherwise Python is not able to free the memory
                // associated to each columns independently.
                // Moreover, one must decrease manually the counter reference for some reason...
                data[header_i] = bp::object(bp::handle<>(PyArray_FROM_OF(valuePyInt, NPY_ARRAY_ENSURECOPY)));
                Py_XDECREF(valuePyInt);
            }

            // Get floats
            Eigen::Matrix<float32_t, Eigen::Dynamic, Eigen::Dynamic> floatDataMatrix;
            floatDataMatrix.resize(timestamps.size(), floatData[0].size());
            for (uint32_t i=0; i<floatData.size(); i++)
            {
                floatDataMatrix.row(i) = Eigen::Matrix<float32_t, 1, Eigen::Dynamic>::Map(
                    floatData[i].data(), floatData[0].size());
            }
            if (clear_memory)
            {
                floatData.clear();
            }

            for (uint32_t i=0; i<floatData[0].size(); i++)
            {
                PyObject * valuePyFloat(getNumpyReferenceFromEigenVector(floatDataMatrix.col(i)));
                std::string const & header_i = header[i + (lastConstantId + 1) + 1 + intData[0].size()];
                data[header_i] = bp::object(bp::handle<>(PyArray_FROM_OF(valuePyFloat, NPY_ARRAY_ENSURECOPY)));
                Py_XDECREF(valuePyFloat);
            }

            return bp::make_tuple(data, constants);
        }

        static bp::tuple getLog(Engine & self)
        {
            std::vector<std::string> header;
            std::vector<float32_t> timestamps;
            std::vector<std::vector<int32_t> > intData;
            std::vector<std::vector<float32_t> > floatData;
            result_t returnCode = self.getLogDataRaw(header, timestamps, intData, floatData);
            if (returnCode == result_t::SUCCESS)
            {
                return formatLog(header, timestamps, intData, floatData);
            }
            else
            {
                return bp::make_tuple(bp::dict(), bp::dict());
            }
        }

        static bp::tuple parseLogBinary(std::string const & filename)
        {
            std::vector<std::string> header;
            std::vector<float32_t> timestamps;
            std::vector<std::vector<int32_t> > intData;
            std::vector<std::vector<float32_t> > floatData;
            result_t returnCode = Engine::parseLogBinaryRaw(filename, header, timestamps, intData, floatData);
            if (returnCode == result_t::SUCCESS)
            {
                return formatLog(header, timestamps, intData, floatData);
            }
            else
            {
                return bp::make_tuple(bp::dict(), bp::dict());
            }
        }

        static bp::dict getOptions(Engine & self)
        {
            bp::dict configPy;
            convertToPy(self.getOptions(), configPy);
            return configPy;
        }

        static result_t setOptions(Engine         & self,
                                   bp::dict const & configPy)
        {
            configHolder_t config = self.getOptions();
            convertToC(configPy, config);
            return self.setOptions(config);
        }

        ///////////////////////////////////////////////////////////////////////////////
        /// \brief Expose.
        ///////////////////////////////////////////////////////////////////////////////
        static void expose()
        {
            bp::class_<Engine,
                       boost::shared_ptr<Engine>,
                       boost::noncopyable>("Engine")
                .def(PyEngineVisitor());
        }
    };
}  // End of namespace python.
}  // End of namespace jiminy.

#endif  // SIMULATOR_PYTHON_H
