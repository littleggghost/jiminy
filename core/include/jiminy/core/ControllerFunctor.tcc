#include <cassert>

#include "jiminy/core/Model.h"


namespace jiminy
{
    template<typename F1, typename F2>
    ControllerFunctor<F1, F2>::ControllerFunctor(F1 & commandFct,
                                                 F2 & internalDynamicsFct) :
    AbstractController(),
    commandFct_(commandFct),
    internalDynamicsFct_(internalDynamicsFct),
    sensorsData_()
    {
        // Empty.
    }

    template<typename F1, typename F2>
    ControllerFunctor<F1, F2>::ControllerFunctor(F1 && commandFct,
                                                 F2 && internalDynamicsFct) :
    AbstractController(),
    commandFct_(std::move(commandFct)),
    internalDynamicsFct_(std::move(internalDynamicsFct)),
    sensorsData_()
    {
        // Empty.
    }

    template<typename F1, typename F2>
    ControllerFunctor<F1, F2>::~ControllerFunctor(void)
    {
        // Empty.
    }

    template<typename F1, typename F2>
    result_t ControllerFunctor<F1, F2>::initialize(std::shared_ptr<Model const> const & model)
    {
        model->getSensorsData(sensorsData_);
        return AbstractController::initialize(model);
    }

    template<typename F1, typename F2>
    result_t ControllerFunctor<F1, F2>::computeCommand(float64_t const & t,
                                                       vectorN_t const & q,
                                                       vectorN_t const & v,
                                                       vectorN_t       & u)
    {
        result_t returnCode = result_t::SUCCESS;

        if (!getIsInitialized())
        {
            std::cout << "Error - ControllerFunctor::computeCommand - The model is not initialized." << std::endl;
            returnCode = result_t::ERROR_INIT_FAILED;
        }

        if (returnCode == result_t::SUCCESS)
        {
            commandFct_(t, q, v, sensorsData_, u);
        }

        return returnCode;
    }

    template<typename F1, typename F2>
    result_t ControllerFunctor<F1, F2>::internalDynamics(float64_t const & t,
                                                         vectorN_t const & q,
                                                         vectorN_t const & v,
                                                         vectorN_t       & u)
    {
        result_t returnCode = result_t::SUCCESS;

        if (!getIsInitialized())
        {
            std::cout << "Error - ControllerFunctor::internalDynamics - The model is not initialized." << std::endl;
            returnCode = result_t::ERROR_INIT_FAILED;
        }

        if (returnCode == result_t::SUCCESS)
        {
            internalDynamicsFct_(t, q, v, sensorsData_, u); // The sensor data are already up-to-date
        }

        return returnCode;
    }
}