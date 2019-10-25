import numpy as np
import tomviz.operators


class RandomTiltSeriesShiftOperator(tomviz.operators.CancelableOperator):

    def transform(self, dataset, maxShift=1):
        """Apply random integer shifts to tilt images"""

        tiltSeries = dataset.active_scalars
        if tiltSeries is None:
            raise RuntimeError("No scalars found!")

        self.progress.maximum = tiltSeries.shape[2]
        step = 0
        for i in range(tiltSeries.shape[2]):
            if self.canceled:
                return
            shifts = (np.random.rand(2) * 2 - 1) * maxShift
            tiltSeries[:, :, i] = np.roll(
                tiltSeries[:, :, i], int(shifts[0]), axis=0)
            tiltSeries[:, :, i] = np.roll(
                tiltSeries[:, :, i], int(shifts[1]), axis=1)
            step += 1
            self.progress.value = step

        dataset.active_scalars = tiltSeries
