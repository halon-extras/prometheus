#include "configuration.hpp"

bool parseConfig(HalonConfig* cfg, ParsedConfig& parsed_cfg)
{
	const char* address = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "address"), nullptr);
	if (address)
		parsed_cfg.address = address;

	const char* port = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "port"), nullptr);
	if (!port)
		port = "9100";
	parsed_cfg.port = (short unsigned int)strtoul(port, nullptr, 0);

	return true;
}
