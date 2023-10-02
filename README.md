# MDADM Library Documentation

Welcome to the MDADM Library documentation! This library provides a simple yet efficient interface for working with JBOD (Just a Bunch Of Disks) storage architecture. It offers various functionalities to interact with the storage system, including caching for improved performance and remote JBOD server connections. The library has been designed to be user-friendly and easy to integrate into your projects.

This documentation covers the following functions:

1. `mdadm_mount()`
2. `mdadm_unmount()`
3. `mdadm_read()`
4. `mdadm_write()`
5. `cache_create()`
6. `cache_destroy()`
7. `jbod_connect()`
8. `jbod_disconnect()`

Read through the detailed function descriptions provided in this document to understand the usage, parameters, and return values of each function. This will ensure a smooth integration of the MDADM Library into your projects.

Please make sure to follow the guidelines and constraints mentioned for each function to avoid any errors or issues while using the library.

Happy coding!

---
### `int mdadm_mount()`

**Description:**

The `mdadm_mount()` function is used to mount the JBOD drives. This function ensures that the drives are only mounted once and properly initializes the system.

**Return values:**

- `1`: The function returns `1` when the JBOD drives are successfully mounted.
- `-1`: The function returns `-1` if the drives are already mounted.

---

### `int mdadm_unmount()`

**Description:**

The `mdadm_unmount()` function is used to unmount the JBOD drives. This function ensures that the drives are unmounted properly and the system is updated accordingly.

**Return values:**

- `1`: The function returns `1` when the JBOD drives are successfully unmounted.
- `-1`: The function returns `-1` if the drives are not currently mounted.

---

### `int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf)`

**Description:**

The `mdadm_read()` function is used to read data from the JBOD storage system. It reads `len` bytes into the `buf` buffer, starting at the linear address `addr`. This function has the following constraints:

- Reading from an out-of-bound linear address should fail.
- A read larger than 1,024 bytes should fail. In other words, `len` can be 1,024 at most.

**Parameters:**

- `uint32_t addr`: The starting linear address to read data from.
- `uint32_t len`: The number of bytes to read.
- `uint8_t *buf`: The buffer to store the read data.

**Return values:**

- `len`: The function returns `len` when the data is read successfully.
- `-1`: The function returns `-1` in case of any error, such as:
  - Drives not mounted.
  - `len` being larger than 1,024 bytes.
  - Out-of-bound linear address.
  - Invalid buffer.

**Usage:**

```c
uint32_t addr = 5000;
uint32_t len = 512;
uint8_t buf[512];

int result = mdadm_read(addr, len, buf);
if (result == len) {
    printf("Data read successfully.\n");
} else {
    printf("Error: Failed to read data.\n");
}
```
---

### `int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf)`

**Description:**

The `mdadm_write()` function is used to write data to the JBOD storage system. It writes `len` bytes from the `buf` buffer, starting at the linear address `addr`. This function has the following constraints:

- Writing to an out-of-bound linear address should fail.
- A write larger than 1,024 bytes should fail. In other words, `len` can be 1,024 at most.

**Parameters:**

- `uint32_t addr`: The starting linear address to write data to.
- `uint32_t len`: The number of bytes to write.
- `const uint8_t *buf`: The buffer containing the data to write.

**Return values:**

- `len`: The function returns `len` when the data is written successfully.
- `-1`: The function returns `-1` in case of any error, such as:
  - Drives not mounted.
  - `len` being larger than 1,024 bytes.
  - Out-of-bound linear address.
  - Invalid buffer.

**Usage:**

```c
uint32_t addr = 5000;
uint32_t len = 512;
const uint8_t buf[512] = { ... };

int result = mdadm_write(addr, len, buf);
if (result == len) {
    printf("Data written successfully.\n");
} else {
    printf("Error: Failed to write data.\n");
}
```
---

### `int cache_create(int num_entries)`

**Description:**

The `cache_create()` function is used to dynamically allocate space for a cache with `num_entries` entries. It stores the address in the `cache` global variable and sets `cache_size` to `num_entries`. This function has the following constraints:

- Calling this function twice without an intervening `cache_destroy()` call should fail.
- The number of cache entries should be between 2 and 4,096, inclusive.

**Parameters:**

- `int num_entries`: The number of cache entries to allocate.

**Return values:**

- `1`: The function returns `1` when the cache is created successfully.
- `-1`: The function returns `-1` in case of any error, such as:
  - Cache already exists.
  - `num_entries` is less than 2 or greater than 4,096.

**Usage:**

```c
int num_entries = 100;

int result = cache_create(num_entries);
if (result == 1) {
    printf("Cache created successfully.\n");
} else {
    printf("Error: Failed to create cache.\n");
}
```
---

### `int cache_destroy()`

**Description:**

The `cache_destroy()` function is used to free the dynamically allocated space for the cache. It sets `cache` to `NULL` and `cache_size` to zero. Calling this function twice without an intervening `cache_create()` call should fail. 

**Parameters:**

- None

**Return values:**

- `1`: The function returns `1` when the cache is destroyed successfully.
- `-1`: The function returns `-1` in case of any error, such as:
  - Cache does not exist.

**Usage:**

```c
int result = cache_destroy();
if (result == 1) {
    printf("Cache destroyed successfully.\n");
} else {
    printf("Error: Failed to destroy cache.\n");
}
```
---
---

### `bool jbod_connect(const char *ip, uint16_t port)`

**Description:**

The `jbod_connect()` function is used to establish a connection to a JBOD server using the given IP address and port number. 

**Parameters:**

- `const char *ip`: The IP address of the JBOD server to connect to.
- `uint16_t port`: The port number of the JBOD server to connect to.

**Return values:**

- `true`: The function returns `true` when the connection is established successfully.
- `false`: The function returns `false` in case of any error, such as:
  - Invalid IP address.
  - Error in socket creation.
  - Error in socket connection.
---

### `void jbod_disconnect(void)`

**Description:**

The `jbod_disconnect()` function is used to close the connection to the JBOD server that was established using the `jbod_connect()` function.

**Parameters:**

This function does not take any parameters.

**Return values:**

This function does not return any values.


---
