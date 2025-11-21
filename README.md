I am developing a module for boredbrain music. It is a software version of the optx eurorack modules. It is a clone of audio8 in vcv rack but with functions for calibrating CV output values.


For calibration I am looking at FFT, since I will have a known pitch target. It will need a very large FFT size for a small bin to achieve high accuracy. 

FFT frequency resolution formula.
Bin Size = Sampling Rate/FFT Size

262144 will give accuracy to < 1 cent (bin size = 0.17Hz at 44.1Khz)

If there are problems, I can try YIN or Autocorrelation. The internet seems to think FFT will work fine in the calibration use case. 


 
