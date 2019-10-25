# FFT (Log Intensity)
# This data transform takes the 3D fourier transform of a 3D dataset
# and returns the FFT's log intensity. The data is normalized such
# that the max intensity is 1.
#
# WARNING: Be patient! Large datasets may take a while.


def transform(dataset):

    import numpy as np

    data_py = dataset.active_scalars

    if data_py is None: # Check if data exists.
        raise RuntimeError("No data array found!")

    offset = np.finfo(float).eps #add a small offset to avoid log(0)

    # Take log abs FFT
    output = np.fft.fftshift(np.log(np.abs(np.fft.fftn(data_py)) + offset))
    # Normalize log abs FFT
    output = output / np.max(output)
    output = np.asfortranarray(output)

    # Set the result as the new scalars.
    dataset.active_scalars = output
