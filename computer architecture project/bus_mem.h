#ifndef BUS_MEM_HEADER
#define BUS_MEM_HEADER

// Bus Codes
#define BUS_NO_CMD_CODE 0
#define BUS_RD_CODE 1
#define BUS_RDX_CODE 2
#define BUS_FLUSH_CODE 3

#define NO_BUS_REQUEST_CODE 0
#define PENDING_SEND_CODE 1
#define WAITING_FLUSH_CODE 2
#define PENDING_WB_SEND_CODE 3

#define EMPTY_DATA_FIELD 0

#define MAIN_MEM_SIZE 1048576 // 2^20

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