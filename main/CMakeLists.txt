set(srcs "main.c" "twai.c")

if(CONFIG_PROTOCOL_TCP)
	list(APPEND srcs "tcp_client.c")
elseif(CONFIG_PROTOCOL_UDP)
	list(APPEND srcs "udp_client.c")
endif()

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")
