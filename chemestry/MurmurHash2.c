// MurmurHash2.c

#include <stdint.h>
#include <stddef.h> 

/**
 * MurmurHash2
 * Función hash rápida que procesa una clave de cualquier longitud.
 * @param key Puntero a la clave de entrada.
 * @param len Longitud de la clave (strlen(key)).
 * @param seed Semilla del hash.
 * @return El valor hash de 32 bits.
 */
uint32_t MurmurHash2(const void * key, int len, uint32_t seed)
{
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    uint32_t h = seed ^ len;

    const unsigned char * data = (const unsigned char *)key;

    // Procesa el cuerpo del mensaje en bloques de 4 bytes
    while (len >= 4)
    {
        uint32_t k = *(uint32_t *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Procesa los bytes restantes (cola)
    switch (len)
    {
    case 3: 
        h ^= data[2] << 16;
        __attribute__ ((fallthrough)); // Fall-through intencional para GCC
    case 2: 
        h ^= data[1] << 8;
        __attribute__ ((fallthrough)); // Fall-through intencional para GCC
    case 1: 
        h ^= data[0];
        h *= m;
    };

    // Cálculos finales
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}