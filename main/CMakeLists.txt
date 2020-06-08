set(COMPONENT_SRCS "sniffer_main.c", "rx_tx_registerer.c", "mesh_io.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_REQUIRES "mqtt")
register_component()
