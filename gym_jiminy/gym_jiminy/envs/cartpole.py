"""
Classic cart-pole system simulated using Jiminy Engine
"""

import os
from math import pi
import numpy as np

from gym import core, spaces, logger
from gym.utils import seeding

import jiminy
from jiminy_py import engine_asynchronous
from gym_jiminy.common import RenderOutMock


class JiminyCartPoleEnv(core.Env):
    """
    Description:
        A pole is attached by an un-actuated joint to a cart.
        The goal is to prevent the pendulum from falling over by increasing and reducing the cart's velocity.
    Observation:
        Type: Box(4)
        Num	Observation                Min         Max
        0	Cart Position             -1.5         1.5
        1	Cart Velocity             -Inf         Inf
        2	Pole Angle                -50 deg      50 deg
        3	Pole Velocity At Tip      -Inf         Inf

    Actions:
        Type: Discrete(2)
        Num	Action
        0	Push cart to the left
        1	Push cart to the right

        Note: The amount the velocity that is reduced or increased is not fixed, it depends on the angle the pole is pointing.
              This is because the center of gravity of the pole increases the amount of energy needed to move the cart underneath it.
    Reward:
        Reward is 1 for every step taken, including the termination step.
    Starting State:
        All observations are assigned a uniform random value in [-0.05..0.05]
    Episode Termination:
        Pole Angle is more than 25 degrees
        Cart Position is more than 75 cm
        Episode length is greater than 200
    Solved Requirements:
        Considered solved when the average reward is greater than or equal to 195.0 over 100 consecutive trials.
    """

    metadata = {
        'render.modes': ['human']
    }

    def __init__(self):
        ############################### Configure the learning #################################

        # The time step of the 'step' method
        self.dt = 2.0e-3

        # Force mag of the action
        self.force_mag = 40.0

        # Angle at which to fail the episode
        self.theta_threshold_radians = 25 * pi / 180
        self.x_threshold = 0.75

        # Angle limit set to 2 * theta_threshold_radians so failing observation is still within bounds
        high = np.array([self.x_threshold * 2,
                         self.theta_threshold_radians * 2,
                         np.finfo(np.float64).max,
                         np.finfo(np.float64).max])


        self.action_space = spaces.Discrete(2) # action can be either 0 or 1
        self.observation_space = spaces.Box(low=-high, high=high, dtype=np.float64)

        ############################# Initialize the simulation ################################

        cur_dir = os.path.dirname(os.path.realpath(__file__))
        urdf_path = os.path.join(cur_dir, "../../../data/cartpole/cartpole.urdf")
        motors = ["slider_to_cart"]
        self.model = jiminy.model()
        self.model.initialize(urdf_path, motors=motors)
        self.model.add_encoder_sensor("Slider", "slider_to_cart")
        self.model.add_encoder_sensor("Pole", "cart_to_pole")
        self.engine_py = engine_asynchronous(self.model)

        ############################# Configure the simulation #################################

        model_options = self.model.get_model_options()
        sensors_options = self.model.get_sensors_options()
        engine_options = self.engine_py.get_engine_options()
        ctrl_options = self.engine_py.get_controller_options()

        model_options["telemetry"]["enableImuSensors"] = False
        engine_options["telemetry"]["enableConfiguration"] = False
        engine_options["telemetry"]["enableVelocity"] = False
        engine_options["telemetry"]["enableAcceleration"] = False
        engine_options["telemetry"]["enableCommand"] = False
        engine_options["telemetry"]["enableEnergy"] = False

        engine_options["stepper"]["solver"] = "runge_kutta_dopri5" # ["runge_kutta_dopri5", "explicit_euler"]
        engine_options["stepper"]["iterMax"] = -1 #Infinite number of iteration
        engine_options["stepper"]["sensorsUpdatePeriod"] = self.dt
        engine_options["stepper"]["controllerUpdatePeriod"] = self.dt

        self.model.set_model_options(model_options)
        self.model.set_sensors_options(sensors_options)
        self.engine_py.set_engine_options(engine_options)
        self.engine_py.set_controller_options(ctrl_options)

        ################################### Miscellaneous ######################################

        self.state = None
        self.viewer = None
        self.steps_beyond_done = None

        self.seed()

    def seed(self, seed=None):
        self.np_random, seed = seeding.np_random(seed)
        self.engine_py.seed(seed)
        self.state = self.engine_py.state
        return [seed]

    def reset(self):
        state = self.np_random.uniform(low=-1, high=1, size=(4,1)) * np.array([[0.5, 0.15, 0.1, 0.1]]).T
        self.engine_py.reset(state)
        self.steps_beyond_done = None
        self.state = self.engine_py.state
        return self.state

    def render(self, lock=None, mode='human'):
        self.engine_py.render(lock)
        if (self.viewer is None):
            self.viewer = self.engine_py._client
        return RenderOutMock()

    def close(self):
        if (self.viewer is not None):
            self.engine_py.close()

    def step(self, action):
        assert self.action_space.contains(action), "%r (%s) invalid"%(action, type(action))

        # Bypass 'self.engine_py.step' method and use direct assignment to max out the performances
        if action == 1:
            self.engine_py._action[0] = self.force_mag
        else:
            self.engine_py._action[0] = -self.force_mag
        self.engine_py.step(dt_desired=self.dt)
        self.state = self.engine_py.state

        # Check the terminal condition and compute reward
        terminal = self._terminal()
        if not terminal:
            reward = 1.0
        elif self.steps_beyond_done is None:
            self.steps_beyond_done = 0
            reward = 1.0
        else:
            if self.steps_beyond_done == 0:
                logger.warn("You are calling 'step()' even though this environment has already returned terminal = True. You should always call 'reset()' once you receive 'terminal = True' -- any further steps are undefined behavior.")
            self.steps_beyond_done += 1
            reward = 0.0

        return self.state, reward, terminal, {}

    def _terminal(self):
        x, theta, x_dot, theta_dot = self.state
        return        x < -self.x_threshold \
               or     x >  self.x_threshold \
               or theta < -self.theta_threshold_radians \
               or theta >  self.theta_threshold_radians