# Práctica 1 – SO - Procesos y comunicación entre procesos

<br>
<img src="https://www.pngkey.com/png/detail/268-2688228_universidad-nacional-colombia-logo.png" width="230" alt="Logo Universidad Nacional de Colombia">
UNIVERSIDAD NACIONAL DE COLOMBIA
<br>
Sistemas Operativos  
2016707 – Grupo 2
<br><br>
Autores:  

* Nicolás Aguirre Velásquez ([niaguirrev@unal.edu.co](mailto:niaguirrev@unal.edu.co))
* Ever Nicolás Muñoz Cortés ([evmunoz@unal.edu.co](mailto:evmunoz@unal.edu.co))
* Fabián David Mora Martínez ([fmoram@unal.edu.co](mailto:fmoram@unal.edu.co))

Docente: **Cesar Augusto Pedraza Bonilla**

---

## Contenido

* [1. Estructura del Proyecto](#1-estructura-del-proyecto)

  * [1.2 Estructura del Dataset](#12-estructura-del-dataset)
  * [1.3 Criterios de Búsqueda](#13-criterios-de-búsqueda)
  * [1.4 Función Hash y Manejo de Colisiones](#14-función-hash-y-manejo-de-colisiones)
* [2. Ejecución Local](#2-ejecución-local)
* [3. Instrucciones de Uso](#3-instrucciones-de-uso)

---

## 1. Estructura del Proyecto

```txt
.
├── DataSet
│   ├── all_movies_expanded.csv        # Dataset expandido (4.6M filas, ~1.3 GB)
│   └── hash_index.bin                 # Archivo binario con la tabla hash generada
├── README.md                          # Este archivo de documentación
├── Makefile                           # Archivo para compilación de los programas
├── murmurHash.c                       # Implementación de la función MurmurHash2 (32 bits)
├── definitions.h                      # Cabecera con estructuras, constantes, etc.
├── index_builder                      # Binario compilado para crear el índice hash
├── index_builder.c                    # Código fuente que construye el índice hash
├── search_process                     # Binario de proceso que realiza búsquedas
├── search_process.c                   # Lógica del proceso de búsqueda
├── ui_process                         # Binario de la interfaz de usuario
└── ui_process.c                       # Interfaz que gestiona la interacción con el usuario
```
Dataset obtenido desde la [API de TheMovieDB](https://developer.themoviedb.org/).

---

### 1.2 Estructura del Dataset

Los campos principales del dataset son:

| Campo              | Descripción                                                              |
| ------------------ | ------------------------------------------------------------------------ |
| **id**             | Identificador único de la película.                                      |
| **title**          | Título de la película (utilizado como clave principal en las búsquedas). |
| **original_title** | Título original en su idioma de origen.                                  |
| **tagline**        | Frase promocional o lema de la película.                                 |
| **overview**       | Sinopsis breve o resumen de la trama.                                    |
| **year**           | Año de lanzamiento.                                                      |
| **director**       | Director de la película.                                                 |
| **runtime**        | Duración (en minutos).                                                   |
| **genres**         | Lista de géneros asociados a la película.                                |
| **vote_average**   | Promedio de votos de los usuarios.                                       |
| **vote_count**     | Número total de votos registrados.                                       |


El dataset original obtenido desde la API de TheMovieDB contenía 577,848 películas únicas registradas entre los años 1900 y 2025, con un tamaño aproximado de 182 MB.
Con el fin de ampliar el volumen de datos y realizar pruebas de rendimiento sobre un dataset de mayor tamaño, con ayuda de una IA se desarrolló un script en Python que generó un dataset expandido aplicando transformaciones controladas sobre los títulos:

Duplicación inversa: cada película fue duplicada invirtiendo el nombre de su título, manteniendo el resto de los campos sin modificaciones.

Inversión parcial: se dividió cada título en dos mitades, invirtiendo su orden y conservando los demás atributos intactos.

Reordenamiento de palabras: las palabras de cada título fueron intercambiadas aleatoriamente para generar nuevas combinaciones válidas.

Tras estas etapas, el dataset resultante alcanzó 4,622,784 registros, con un peso final de aproximadamente 1.3 GB, conservando la coherencia estructural de los datos originales.

**Tamaño final del dataset:** 4,622,784 filas (~1.3 GB)<br>
**Campo de indexación:** `title` (15 caracteres normalizados)<br>
**Función hash:** MurmurHash2 – 32 bits<br>
**Manejo de colisiones:** Encadenamiento dinámico en archivo binario<br>
**Tipo de comunicación entre procesos:** Memoria compartida<br>

---

### 1.3 Criterios de Búsqueda

El campo utilizado para las búsquedas fue **`title`**, ya que es el atributo más representativo y distintivo para identificar una película.
Se toman **los primeros 15 caracteres del título**, convertidos a minúsculas, como clave de hashing.
Esto permite mantener una clave consistente y corta, manteniendo un número de colisiones decente y mejorando el rendimiento de búsqueda.

Ejemplo:

```
"The Matrix" → "the matrix"
"Fast And Furious" → "fast and furiou"
```

De esta forma, títulos similares o con pequeñas variaciones no generan hash completamente diferentes, ayudando a una búsqueda más eficiente.

---

### 1.4 Función Hash y Manejo de Colisiones

Se empleó la función **MurmurHash2 (32 bits)** con el algoritmo original, debido a su alta uniformidad de dispersión y velocidad.
El hash resultante se utiliza para indexar cada película dentro de una **tabla hash almacenada en un archivo binario** (`hash_index.bin`).

#### 🔹 Estrategia de colisiones:

Cuando varias películas comparten el mismo hash:

1. Si la posición está libre, se inserta directamente.
2. Si ya existe una entrada, se crea una nueva al final del archivo binario.
3. El registro original actualiza su puntero `next` para apuntar al nuevo.
4. Si ese nuevo también tiene colisión, se repite el proceso, formando una **cadena enlazada de colisiones en disco**.

De esta forma, nunca se sobrescriben registros y todas las entradas se mantienen accesibles mediante recorrido de la cadena.

---

## 2. Ejecución Local

1. Clonar el repositorio:

   ```bash
   git clone https://github.com/niko2k7/Practica1_SO.git
   ```
2. Compilar los archivos:

   ```bash
   make
   ```
3. Crear el índice hash:

   ```bash
   ./index_builder
   ```
4. Ejecutar los procesos:

   ```bash
   ./ui_process
   ./search_process
   ```

---

## 3. Instrucciones de Uso

* Al ejecutar `ui_process`, el sistema inicia el proceso de la interfaz.
* Al ejecutar `search_process`, el sistema inicia el proceso de búsqueda y se queda en espera.
* El usuario ingresa un título de película al seleccionar la opción 1 en el proceso de la interfaz.
* El usuario selecciona la opción 2 para empezar la búsqueda y comunicar ambos procesos.
* El proceso de búsqueda localiza su posición en el índice hash.
* Se muestra la información completa si se encuentra, o un mensaje de no coincidencia (NA).
* Se puede repetir el proceso, o seleccionar la opción 3 para salir.

---

> *Proyecto desarrollado como práctica de manejo de procesos, memoria compartida y estructuras de datos aplicadas a sistemas operativos.*
