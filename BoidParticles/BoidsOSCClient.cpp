#include "BoidParticles.h"

BoidsOSCClient::BoidsOSCClient(Particles *particles)
{
	this->particles = particles;
}

void BoidsOSCClient::ProcessMessage(const osc::ReceivedMessage & message, const IpEndpointName & remoteEndpoint)
{
	std::cout << "gotmessage" <<std::endl;
	try {

		if (std::strcmp(message.AddressPattern(), "/setgoal") == 0) {
			osc::ReceivedMessageArgumentStream args = message.ArgumentStream();
			float goal;
			const char *name;
			args >> goal >> name >> osc::EndMessage;

			particles->updateGoal((int)goal);

			std::cout << "setting goal to" << (int)goal << "/n";
		}
	}
	catch (osc::Exception& e) {
		std::cout << "error while parsing message: "
			<< message.AddressPattern() << ": " << e.what() << "\n";
	}

}


