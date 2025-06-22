import numpy as np



def _calc_triag(pt0: tuple, pt1: tuple, pt2: tuple) -> float:
    """
    Triangle area based on points.
    https://en.wikipedia.org/wiki/Area_of_a_triangle

    :param pt: tuple of position

    :return: area of triangle
    """
    
    xa, ya = pt0
    xb, yb = pt1
    xc, yc = pt2

    return 0.5*abs((xa-xc)*(yb-ya) - (xa-xb)*(yc-ya))



def _average_angle(phi0: float, phi1: float):
    """
    Calculate the middle of an angle. Simply doing (phi1+phi2)/2 can yield
    unxpexted results. For example if phi1 is in 2. quadrant and phi2 in 3.
    quandrat etc.
    """
    x = np.cos(phi0) + np.cos(phi1)
    y = np.sin(phi0) + np.sin(phi1)

    return np.atan2(y, x)



def _calc_trajectory(pt0, pt1, eps: float = 5) -> list[list[float]]:
    """
    Calculate the minimal required steps to make a straight line on the sand
    table based on the desired accuracy.

    :param pt0: tuple of (x0, y0)
    :param pt1: tuple of (x1, y1)
    :param eps: desired accurary. Calculated as triangle area between three
        points, where the three points are the start and end of the line and
        the middle arc point.

    :return: list of positions with shape [n, 2]
    """
    x0, y0 = pt0
    x1, y1 = pt1
    r0, phi0 = np.sqrt(x0**2+y0**2), np.atan2(y0, x0)
    r1, phi1 = np.sqrt(x1**2+y1**2), np.atan2(y1, x1)
    
    r01 = (r0+r1)/2
    # phi01 = (phi0+phi1)/2
    phi01 = _average_angle(phi0, phi1)
    x01p, y01p = r01*np.cos(phi01), r01*np.sin(phi01)

    A = _calc_triag((x0, y0), (x01p, y01p), (x1, y1))

    x01, y01 = (x0+x1)/2, (y0+y1)/2

    if A > eps:
        new_seg0 = _calc_trajectory(pt0, (x01, y01), eps)
        new_seg1 = _calc_trajectory((x01, y01), pt1, eps)

        ret_list = [*new_seg0, *new_seg1[1:]]

    else:
        ret_list = [pt0, (x01, y01), pt1]

    return ret_list



class PathMaker:
    """
    A Generator class that holds the trajectory points. Once instantiated
    use next(instance) to receive the next valid point. Once the end of the
    generator is reached, it will yield None.

    Available attributes
    - calc_pts -> calculated points in XY CS
    - pts_polar -> calculated point in polar CS

    :param pts: input points as np.array([N,2]) in mm
    :param eps: desired accuracy of the output trajectory. If None, the input
        path will remain as is.
    :param rot_angle: rotate the path for this angle for the next run in 
        degrees
    :param num_iterations: Repeat the input path n times. When the end is
        reached the generator will yield a None value.
    """
    RADIUS_LIMIT_MM = 250 # max allowed r distance in mm
    RADIUS_STEPS_MM = 81.82 # steps per mm for the radial position
    ANGLE_STEPS_RAD = 4169.86 # steps per radian of rotation


    def __init__(self, pts: np.ndarray, eps: float = None, 
                 rot_angle: float = 5, num_iterations: int = 1):
        self.pts = pts
        self.eps = eps
        # convert angle in degree to radians and then to number of steps
        self.rot_steps = int(rot_angle*np.pi/180*self.ANGLE_STEPS_RAD)
        self.num_iterations = num_iterations
        self.iter_counter = 0


        # check radius limits
        if not np.all(self.pts[:,0] < self.RADIUS_LIMIT_MM):
            raise ValueError(f"Max allowed R value is {self.RADIUS_LIMIT_MM}")

        self._get_new_pts()
        self._calc_positions()


    def _get_new_pts(self) -> None:
        """
        Calculate the required trajectory points based on input points.
        """
        if self.eps is None:
            calc_pts = self.pts
        if isinstance(self.eps, (float, int)):
            for i in range(self.pts.shape[0]-1):
                ret = _calc_trajectory(self.pts[i], self.pts[i+1], self.eps)
                ret = np.array(ret)

                if i == 0:
                    calc_pts = ret
                else:
                    calc_pts = np.vstack((calc_pts, ret[1:]))
        else:
            raise TypeError(f"eps can only be of type int, float or None" \
                            f" and not {type(self.eps)}")

        r = np.sqrt(calc_pts[:,0]**2 + calc_pts[:,1]**2)
        phi = np.atan2(calc_pts[:,1], calc_pts[:,0]).T

        self.pts_polar = np.vstack((r, phi)).T


    def _calc_positions(self) -> None:
        """
        Based on self.pts_polar the required positions in steps are
        calculated.
        """
        self.positions = np.zeros_like(self.pts_polar)

        self.positions[:, 0] += self.pts_polar[:, 0]*self.RADIUS_STEPS_MM
        self.positions[1:, 1] += np.diff(
            np.unwrap(self.pts_polar[:,1]))*self.ANGLE_STEPS_RAD

        self.positions = self.positions.astype(np.int32)

        self._pts_size = self.positions.shape[0]
        self._current_position = 0

    
    def __next__(self):
        if self._current_position == self._pts_size:
            self._current_position = 0
            return np.array([self.positions[0,0], self.rot_steps])
        
        next_pt = self.positions[self._current_position]
        self._current_position += 1

        return next_pt


    def __iter__(self):
        return self

    