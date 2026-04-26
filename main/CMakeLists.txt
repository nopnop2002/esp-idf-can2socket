set(srcs "main.c")

if (IDF_VERSION_MAJOR STREQUAL "5")
	list(APPEND srcs "twai_task_v5.c")
elseif (IDF_VERSION_MAJOR STREQUAL "6")
	list(APPEND srcs "twai_task_v6.c")
endif()

if(CONFIG_PROTOCOL_TCP)
	list(APPEND srcs "tcp_client.c")
elseif(CONFIG_PROTOCOL_UDP)
	list(APPEND srcs "udp_client.c")
endif()

idf_component_register(SRCS ${srcs} INCLUDE_DIRS ".")
