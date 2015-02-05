#!/usr/bin/env python

# import sys
# import pygsl as gsl
# import pygsl.fft as fft
# import pygsl.spline as spline
# import pygsl.wavelet as wvlt
# import math as M
import numpy as N
# import scipy as S

class HilbertTransformer():

    def __init__(self):
        self.x = N.zeros(4)
        self.y = N.zeros(6)
        self.d = N.zeros(6)

    def reset(self):
        self.x[:] = 0
        self.y[:] = 0
        self.d[:] = 0

    def tick(self, xin):
        self.x[0] = self.d[1] - xin
        self.x[1] = self.d[0] - self.x[0] * 0.00196
        self.x[2] = self.d[3] - self.x[1]
        self.x[3] = self.d[1] + self.x[2] * 0.737
        
        self.d[1] = self.x[1]
        self.d[3] = self.x[3]
        
        self.y[0] = self.d[2] - xin
        self.y[1] = self.d[0] + self.y[0] * 0.924
        self.y[2] = self.d[4] - self.y[1]
        self.y[3] = self.d[2] + self.y[2] * 0.439
        self.y[4] = self.d[5] - self.y[3]
        self.y[5] = self.d[4] - self.y[4] * 0.586
        
        self.d[2] = self.y[1]
        self.d[4] = self.y[3]
        self.d[5] = self.y[5]
        
        self.d[0] = xin

        return complex(self.x[3], self.y[5])


    def run(self, xa):
        return N.asarray(map(lambda x: self.tick(x), xa))
        

if __name__ == '__main__':
    pass
