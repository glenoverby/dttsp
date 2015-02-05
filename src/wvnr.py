#!/usr/bin/env python

import sys
import pygsl as gsl
import pygsl.fft as fft
import pygsl.spline as spline
import pygsl.wavelet as wvlt
import math as M
import numpy as N
import scipy as S

import aux as A
import hilbert as H

class WavOla():
    def __init__(self,
                 order = 20,
                 size  = 1024):
        self.size = size
        self.half = A.nblock2(size)
        self.wlen = self.half*2

        self.wvlt_i = wvlt.daubechies(order)
        self.wwrk_i = wvlt.workspace(self.wlen)
        self.tail_i = N.zeros(self.half)

        self.wvlt_q = wvlt.daubechies(order)
        self.wwrk_q = wvlt.workspace(self.wlen)
        self.tail_q = N.zeros(self.half)

#       self.hilb = H.HilbertTransformer()

    def run(self, x, mask):
        self.xdat_i = N.concatenate((x[0], N.zeros(self.half)))
        self.wdat_i = self.wvlt_i.transform_forward(self.xdat_i, self.wwrk_i)
        self.wdat_i *= mask
        self.ydat_i = self.wvlt_i.transform_inverse(self.wdat_i, self.wwrk_i)
        self.odat_i = self.ydat_i[         :self.half] + self.tail_i
        self.tail_i = self.ydat_i[self.half:         ].copy()

        self.xdat_q = N.concatenate((x[1], N.zeros(self.half)))
        self.wdat_q = self.wvlt_q.transform_forward(self.xdat_q, self.wwrk_q)
        self.wdat_q *= mask
        self.ydat_q = self.wvlt_q.transform_inverse(self.wdat_q, self.wwrk_q)
        self.odat_q = self.ydat_q[         :self.half] + self.tail_q
        self.tail_q = self.ydat_q[self.half:         ].copy()

        return array([self.odat_i, self.odat_q])
