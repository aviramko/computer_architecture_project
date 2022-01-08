#ifndef BUS_MEM_HEADER
#define BUS_MEM_HEADER

typedef enum bus_requests {
	no_cmd,
	bus_rd,
	bus_rdx,
	flush
} bus_requests;

typedef struct memory_address
{
	unsigned int index : 8;
	unsigned int tag : 12;

} address;

typedef struct MSI_bus
{
	unsigned int bus_shared : 1;
	unsigned int bus_data : 32;
	address bus_addr;
	unsigned int bus_cmd : 2;
	unsigned int bus_origid : 3;

} msi_bus;

//void initialize_main_mem_and_bus_requests(int *main_mem, msi_bus *memory_bus_request, msi_bus empty_request);
void initialize_bus(msi_bus *bus);
void initialize_main_mem(int *main_mem);


#endif // !BUS_MEM_HEADER