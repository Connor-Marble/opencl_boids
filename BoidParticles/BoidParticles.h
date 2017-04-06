#pragma once

#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <GL\glew.h>
#include <GL\glut.h>
#include <CL\cl.hpp>
#include <sstream>

void CreateOpenGL(int argc, char** argv);
void CheckError(int code);

typedef struct Vector4 {
	float x, y, z, w;
	Vector4() {
		x = 0;
		y = 0;
		z = 0;
		w = 0;
	};

	Vector4(float ix, float iy, float iz, float iw) {
		x = ix;
		y = iy;
		z = iz;
		w = iw;
	}
};

class Particles {
	public:
		int PARTICLE_NUM;

		std::vector<cl::Memory> vbos;
		std::vector<Vector4> *pos_vecs;
		std::vector<Vector4> *vel_vecs;

		cl::Buffer pos, vel, startpos, startvel;
		Vector4 *hostvecs;
		int pos_vbo, vel_vbo;

		size_t arr_size;

		Particles(int particle_count);

		void BufferSetup(int size);
		
		~Particles();

		void LoadKernel();

		void run();

		int x, y;

		int framecount = 0;
		double laststart = 0.0;

	private:
		std::vector<cl::Device> devices;

		cl::CommandQueue commQ;
		cl::Context context;
		cl::Kernel kernel;
		cl::Program program;

		UINT clDevice;

};