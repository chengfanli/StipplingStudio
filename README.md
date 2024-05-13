<img src="./teaser.png" alt="teaser" style="zoom:50%;" />

# Stippling

CSCI 2240 Final Project for the Spring 2024 by Stippling Studio ([Chengfan Li](https://github.com/lllllcf), [Wendi Liao](https://github.com/wendi-liao) and [Yixuan Liu](https://github.com/Ahhhh2016))

Special thanks to the papers "[Weighted Linde-Buzo-Gray Stippling](https://graphics.uni-konstanz.de/publikationen/Deussen2017LindeBuzoGray/index.html)" and "[Multi-class Inverted Stippling](https://kops.uni-konstanz.de/bitstreams/21672707-75c4-410b-a4ff-87b21c2ed630/download)" that inspired this project.

\[[Demo Video](https://drive.google.com/file/d/1Hk2hPs_mVhhbz9oe8zTLfxnnQoMRPXu8/view?usp=drive_link)\]

## Overview

Based on the research and exploration of stippling, we have implemented the following features.

+ Weighted LBG algorithm based stippling that can convert grayscale image to stippling.
+ Visualization of Voronoi diagrams, as well as splitting and merging of points.
+ Video stippling with low noise and minimal flickering using WLGB.
+ Multi-class Inverted Stippling for grayscale image.
+ Color Stippling with palette extraction.
+ UNet-based reconstruction model to restore stippling to grayscale images.

## WLBG

For more information, please read the README for WLBG.



## MLBG

For more information, please read the README for MLBG.



## Reconstruction

For more information, please read the README for Reconstruction.



## Gallery

<img src="Gallery/book.gif" width="800">

<img src="Gallery/monroe_with_ordering.png" width="300">    <img src="Gallery/filling.jpg" width="500">

<img src="Gallery/s_0378.png" width="400">    <img src="Gallery/s_1547.png" width="400">

<img src="Gallery/color_with_filling.jpg" width="600">

 <img src="Gallery/s_0019.png" width="400"> <img src="Gallery/s_1503.png" width="400">

## References

Deussen, Oliver, Marc Spicker, and Qian Zheng. "Weighted linde-buzo-gray stippling." *ACM Transactions on Graphics (TOG)* 36.6 (2017): 1-12.

Christoph Schulz, Kin Chung Kwan, Michael Becher, Daniel Baumgartner, Guido Reina, Oliver Deussen, and Daniel Weiskopf. 2021. Multi-Class Inverted Stippling. *ACM Trans*. Graph. 40, 6 (2021)







