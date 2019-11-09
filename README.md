# 3D-Virutal-World-to-Benchmark-GPUs
Project for the course Computer Graphics and Product Modelling

to track gpu usage - use
nvidia-smi dmon -i 0 -s mu -d 1 -o TD

to write it to a text file use
nvidia-smi dmon -i 0 -s mu -d 1 -o TD >gpu_log.txt

for the virtual world, after installing the required packages, read the README file in that folder and then simply build using:

./build.sh

for extra gpu load, compule the cuda file 
gpu_load.cu
run it and then enter the desired load in MB