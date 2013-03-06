
Refuzznik is a distortion effect that applies a sinusoidal waveshaping
function to its input signal(s). Sinusoidal distortion can be mixed with
traditional shelving distortion. The relationship between input signal
level and sinusoid frequency can be adjusted.

---<<< Parameters >>>---
--- ZeroX ---
ZeroX (short for "add zero crossings") is an on/off parameter that
affects the shape of the sinusoid function. If ZeroX = "No", the
sinusoid will map each input value to a value with the same sign
(positive to positive, negative to negative). If ZeroX = "Yes", the
sinusoid may map a positive value to a negative one and vice versa.

--- InGain ---
This parameter determines how much Refuzznik amplifies the input signal
before applying the distortion function. An increase in amplification
results in louder and/or more distorted sound.

--- SlewLim ---
The slew limiter places an upper bound on changes in the input to the
sinusoidal distortion unit. The value of the input cannot change by more
than SlewLim percent of the maximum level from one sample to the next.
Slew limiting reduces the noisiness of the output when the frequency
settings (see below) are high.

--- FuncMix ---
The FuncMix parameter is used to set the mix of sinusoidal and shelving
distortion. At FuncMix = 0%, only shelving distortion is used. At
FuncMix = 100%, only sinusoidal distortion is used.

--- AmpMin ---
This parameter is used to set the minimum amplitude of the sinusoidal
function. When the signal level of the input is zero, the amplitude of
the sinusoid is equal to AmpMin. The sinusoid amplitude increases with
the input signal level and will approach the maximum "normal" VST signal
level (which is 1) when the input level exceeds that maximum.

--- FreqMin ---
This parameter is used to set the minimum frequency of the sinusoidal
function. When the signal level of the input is zero, the frequency of
the sinusoid is equal to FreqMin. An increase in frequency usually
results in harsher, noisier sinusoidal distortion.

NOTE: Sinusoid frequencies are specified as the number of cycles the
sinusoid will go through when the input signal level increases linearly
from the minimum value (-1) to the maximum value (1).

--- FreqDif ---
The FreqDif parameter determines how much the frequency
of the sinusoidal function will change when the input signal level goes
from zero to the VST maximum. If FreqDif > 0, the frequency will
increase with the input level; if FreqDif < 0, the frequency will
decrease.

--- OutGain ---
This parameter specifies how much the volume of the distorted signal is
reduced before it is sent to the output. Turn this down when you want a
sound that is heavily distorted but still not incredibly loud.
