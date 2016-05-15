import scipy.optimize
import skimage.io
import skimage.color
import numpy as np
import matplotlib.pyplot as plt

def linear2srgb(yy):
    t = 0.0031308
    yy[yy>t] = 1.055*np.power(yy[yy>t],1.0/2.4)-0.055
    yy[yy<=t] = yy[yy<=t]*12.92
    return yy

O = skimage.io.imread("mcm_5.tiff")
O = skimage.img_as_float(O)

xyz = np.zeros((1,1,3))
xyz[...] = [100,0,0]
xyz /= 100.0
print xyz, 255*skimage.color.xyz2rgb(xyz)

xx = np.linspace(0,1,500)
yy = np.copy(xx)
yy = linear2srgb(yy)

chans = ['r','g','b']
for chan in range(3):
    yy2 = O[0,:,chan]
    plt.plot(xx, yy2, c=chans[chan])
plt.plot(xx, yy, c='black')
plt.xlim([0,1])
plt.ylim([0,1])
plt.plot([0,1], [0,1], c='black')
plt.gca().set_aspect('equal', adjustable='box')
plt.savefig("map.pdf")
