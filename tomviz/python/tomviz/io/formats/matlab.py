# -*- coding: utf-8 -*-

###############################################################################
# This source file is part of the Tomviz project, https://tomviz.org/.
# It is released under the 3-Clause BSD License, see "LICENSE".
###############################################################################
import numpy as np
import scipy.io

from tomviz.io import FileType, IOBase, Reader

import tomviz.utils

from vtk import vtkImageData


class MatlabBase(IOBase):

    @staticmethod
    def file_type():
        return FileType('MATLAB binary format', ['mat'])


class MatlabReader(Reader, MatlabBase):

    def read(self, path):
        mat_dict = scipy.io.loadmat(path)

        data = None
        for item in mat_dict.values():
            # Assume only one 3D array per file
            if isinstance(item, np.ndarray):
                if len(item.shape) == 3:
                    data = item
                    break

        if data is None:
            return vtkImageData()

        image_data = vtkImageData()
        (x, y, z) = data.shape
        image_data.SetOrigin(0, 0, 0)
        image_data.SetSpacing(1, 1, 1)
        image_data.SetExtent(0, x - 1, 0, y - 1, 0, z - 1)
        tomviz.utils.set_array(image_data, data)

        return image_data
