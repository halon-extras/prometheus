#include <HalonMTA.h>
#include "configuration.hpp"
#include <stdexcept>
#include <string>
#include <array>
#include <microhttpd.h>
#include <cstring>
#include <syslog.h>
#include <arpa/inet.h>

#if MHD_VERSION < 0x00097002
typedef int MHD_Result;
#endif

static ParsedConfig parsed_cfg;
static struct MHD_Daemon* mhd_daemon;

HALON_EXPORT
int Halon_version()
{
	return HALONMTA_PLUGIN_VERSION;
}

MHD_Result send_text_response(struct MHD_Connection* connection, const char* page, int status_code)
{
	struct MHD_Response* response = MHD_create_response_from_buffer(strlen(page), (void*)page, MHD_RESPMEM_MUST_COPY);
	if (!response)
		return MHD_NO;
	MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");
	MHD_add_response_header(response, MHD_HTTP_HEADER_SERVER, "Halon");
	MHD_Result ret = MHD_queue_response(connection, status_code, response);
	MHD_destroy_response(response);
	return ret;
}

MHD_Result request_process(void* cls, struct MHD_Connection* connection,
						   const char* url, const char* method,
						   const char* version, const char* upload_data,
						   size_t* upload_data_size, void** con_cls)
{
	if (strcmp(url, "/metrics") != 0)
		return send_text_response(connection, "404 Not Found", MHD_HTTP_NOT_FOUND);

	if (strcmp(method, "GET") != 0)
		return send_text_response(connection, "405 Method Not Allowed", MHD_HTTP_METHOD_NOT_ALLOWED);

	std::string metrics = "";
	try
	{
		std::array<char, 128> buffer{};

		FILE* pipe = popen("/opt/halon/bin/halonctl process-stats --openmetrics", "r");
		if (!pipe)
			throw std::runtime_error("popen() failed");

		while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
			metrics += buffer.data();

		int status = pclose(pipe);
		if (status == -1)
			throw std::runtime_error("pclose() failed");

		if (WIFEXITED(status))
		{
			int s = WEXITSTATUS(status);
			if (s != 0)
				throw std::runtime_error("command failed");
		}
		else
			throw std::runtime_error("command failed");
	}
	catch (const std::runtime_error& e)
	{
		syslog(LOG_CRIT, "prometheus: %s", e.what());
		return send_text_response(connection, "500 Internal Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	return send_text_response(connection, metrics.c_str(), MHD_HTTP_OK);
}

HALON_EXPORT
bool Halon_init(HalonInitContext* hic)
{
	HalonConfig* cfg = nullptr;
	HalonMTA_init_getinfo(hic, HALONMTA_INIT_CONFIG, nullptr, 0, &cfg, nullptr);
	if (!parseConfig(cfg, parsed_cfg))
		return false;

	struct sockaddr_storage dst;
	memset(&dst, 0, sizeof dst);

	unsigned int flags = MHD_USE_THREAD_PER_CONNECTION;
	if (parsed_cfg.address.empty())
		flags |= MHD_USE_DUAL_STACK;
	else
	{
		char buf[sizeof(struct in6_addr)];
		memset(buf, 0, sizeof buf);

		if (inet_pton(AF_INET, parsed_cfg.address.c_str(), &buf) == 1)
		{
			dst.ss_family = AF_INET;
			sockaddr_in* d = (sockaddr_in*)&dst;
			memcpy(&(d->sin_addr), buf, sizeof(in_addr));
			d->sin_port = htons(parsed_cfg.port);
		}
		else if (inet_pton(AF_INET6, parsed_cfg.address.c_str(), &buf) == 1)
		{
			dst.ss_family = AF_INET6;
			sockaddr_in6* d = (sockaddr_in6*)&dst;
			memcpy(&(d->sin6_addr), buf, sizeof(in6_addr));
			d->sin6_port = htons(parsed_cfg.port);
			flags |= MHD_USE_IPv6;
		}
		else
		{
			syslog(LOG_CRIT, "prometheus: bad address (%s)", parsed_cfg.address.c_str());
			return false;
		}
	}

	mhd_daemon = MHD_start_daemon(flags,
								  parsed_cfg.port,
								  nullptr,
								  nullptr,
								  &request_process,
								  nullptr,
								  MHD_OPTION_SOCK_ADDR, !parsed_cfg.address.empty() ? &dst : NULL,
								  MHD_OPTION_END);
	if (!mhd_daemon)
		return false;

	syslog(LOG_CRIT, "prometheus: listening on %s:%d", !parsed_cfg.address.empty() ? parsed_cfg.address.c_str() : "*", parsed_cfg.port);

	return true;
}

HALON_EXPORT
void Halon_early_cleanup()
{
	MHD_stop_daemon(mhd_daemon);
}
