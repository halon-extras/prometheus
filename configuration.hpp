#ifndef CONFIGURATION_HPP_
#define CONFIGURATION_HPP_

#include <HalonMTA.h>
#include <string>

struct ParsedConfig
{
	std::string address;
	short unsigned int port;
};

bool parseConfig(HalonConfig* cfg, ParsedConfig& parsed_cfg);

#endif
