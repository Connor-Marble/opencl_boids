// BoidParticles.cpp : Defines the entry point for the console application.
//

#include "BoidParticles.h"

#define NORTH  0
#define EAST   1
#define SOUTH  2
#define WEST   3
#define CENTER 4


Particles* particles;
std::mutex netmutex;

void runClient(Particles *particles)
{

	BoidsOSCClient client(particles);
	UdpListeningReceiveSocket sock(IpEndpointName(IpEndpointName::ANY_ADDRESS, PORT), &client);
	std::cout << "starting socket";
	sock.RunUntilSigInt();
};


int main(int argc, char** argv)
{
	
	
	//display setup
	CreateOpenGL(argc, argv);
	particles = new Particles(4000);
	signed int size = sizeof(Vector4) * (particles->PARTICLE_NUM);
	particles->BufferSetup(size);

	//osc setup
	std::thread netThread(runClient, particles);

	particles->LoadKernel();
	glutMainLoop();

	netThread.join();

    return 0;
}



void render() {

	particles->framecount++;
	if (particles->framecount % 100 == 0) {
	std::cout << (float)particles->framecount / (float)glutGet(GLUT_ELAPSED_TIME) * 1000.0f << std::endl;
	}
	glPushMatrix();

	
	glRotatef((GLfloat)((float)(particles->x) / 2.0f) - 50.0f, 0.0f, 1.0f, 0.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH);

	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)particles->col_vbo);
	glColorPointer(4, GL_FLOAT, 0, 0);

	glPointSize(2.0f);
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)particles->pos_vbo);
	glVertexPointer(4, GL_FLOAT, 0, 0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glDrawArrays(GL_POINTS, 0, particles->PARTICLE_NUM);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glutSwapBuffers();
	glPopMatrix();

	particles->run();
}

void appMotion(int x, int y)
{
	particles->x = x;
	particles->y = y;
}

void CreateOpenGL(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutInitWindowPosition(0, 0);
	
	glutCreateWindow("BoidTest");
	glutDisplayFunc(render);
	glutIdleFunc(render);
	glutMotionFunc(appMotion);
	glewInit();

	glViewport(0, 0, 512, 512);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective((GLfloat)90.0, (GLfloat)1.0, 0.1, 500.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -7.0);

	glColor3f(1.0,0.6,0.8);
}

Particles::Particles(int particle_count)
{

	hostvecs = new Vector4[particle_count];
	int error=0;
	PARTICLE_NUM = particle_count;
	std::vector<cl::Platform> platforms;
	int platformid = 0;

	cl::Platform::get(&platforms);

	platforms[platformid].getDevices(CL_DEVICE_TYPE_GPU, &devices);

	pos_vecs = new std::vector<Vector4>;
	col_vecs = new std::vector<Vector4>;

	for (int i = 0; i < particle_count; i++) {
		pos_vecs->push_back(Vector4((float)(std::rand() / 4000.0f - 5.0f) , (float)(std::rand() / 2000.0f - 10.0f), (float)(std::rand() / 2000.0f - 10.0f), 1.0f));
		col_vecs->push_back(Vector4((float)std::rand()/15000.0f, (float)std::rand() / 15000.0f, (float)std::rand() / 15000.0f, 1.0f));
	}


	cl_context_properties props[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[platformid])(),
		0
	};


	context = cl::Context(CL_DEVICE_TYPE_GPU, props,NULL,NULL,&error);
	CheckError(error);


	commQ = cl::CommandQueue(context, devices[0], 0, &error);
	CheckError(error);
}

void Particles::BufferSetup(int size) {

	int errcode = 0;

	GLuint pid, cid;

	glGenBuffers(1, &pid);

	glBindBuffer(GL_ARRAY_BUFFER, pid);
	glBufferData(GL_ARRAY_BUFFER, size, &((*pos_vecs)[0]),GL_DYNAMIC_DRAW);

	

	pos_vbo = pid;
	vbos.push_back(cl::BufferGL(context, CL_MEM_READ_WRITE, pos_vbo, &errcode));
	CheckError(errcode);

	col_vbo = 0;

	glGenBuffers(1, &cid);
	glBindBuffer(GL_ARRAY_BUFFER, cid);
	glBufferData(GL_ARRAY_BUFFER, size, &((*col_vecs)[0]), GL_DYNAMIC_DRAW);
	col_vbo = cid;

	CheckError(errcode);
	vel = cl::Buffer(context, CL_MEM_WRITE_ONLY, size, NULL, &errcode);
	CheckError(errcode);

}

Particles::~Particles()
{
}

void Particles::LoadKernel()
{
	int error = 0;
	std::ifstream stream("KernelSource.cl");
	std::stringstream buffer;
	buffer << stream.rdbuf();
	std::string source = buffer.str();
	std::string line;


	do {
		stream >> line;
		source += line + " ";
	} while (! stream.eof());


	int sourceSize = source.size();

	cl::Program::Sources sourceObj(1,std::make_pair( source.c_str(),sourceSize));
	

	program = cl::Program(context, sourceObj, &error);
	CheckError(error);

	CheckError(program.build(devices));

	kernel = cl::Kernel(program, "KernelSource", &error);
	CheckError(error);

	CheckError(kernel.setArg(1, vel));
	CheckError(kernel.setArg(0, vbos[0]));


	commQ.finish();
}

void Particles::run()
{

	glFinish();
	int err = 0;
	CheckError(commQ.enqueueAcquireGLObjects(&vbos, NULL, NULL));
	commQ.finish();
	float dt = ((float)glutGet(GLUT_ELAPSED_TIME) * 1000.0f) / (float)particles->framecount;
	kernel.setArg(2, dt); 

	if (p_mutex.try_lock()) {
		kernel.setArg(3, goal);
		p_mutex.unlock();
	}

	CheckError(commQ.enqueueNDRangeKernel(kernel, 0, cl::NDRange(PARTICLE_NUM), cl::NullRange, NULL, NULL));
	commQ.finish();

	CheckError(commQ.enqueueReleaseGLObjects(&vbos, NULL, NULL));
	commQ.finish();
	
}

void Particles::updateGoal(int goalset)
{
	p_mutex.lock();
	goal = goalset;
	p_mutex.unlock();
}

void CheckError(int code) {
	if (code != 0) {
		throw std::exception("OpenCL error " + code);
	}
}