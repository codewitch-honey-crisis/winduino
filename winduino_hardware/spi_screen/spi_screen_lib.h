#include <stdint.h>
#include <stddef.h>

#define DLL_API __declspec(dllexport)

#define CALL __cdecl

typedef CALL void(*gpio_set_callback)(uint32_t value, void* state);
typedef CALL uint8_t(*gpio_get_callback)(void* state);
typedef CALL void(*log_callback)(const char* value);
#ifdef __cplusplus
extern "C"
{
#endif

DLL_API int CALL Configure(int prop, void* data, size_t size);
DLL_API int CALL Connect(uint8_t pin, gpio_get_callback getter, gpio_set_callback setter, void* state);
//DLL_API int CALL Update();
DLL_API int CALL PinChange(uint8_t pin, uint32_t value);
DLL_API int CALL TranferBitsSPI(uint8_t* data, size_t size_bits);
DLL_API int CALL AttachLog(log_callback logger);
#ifdef __cplusplus
} // __cplusplus defined.
#endif