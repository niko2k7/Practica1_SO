# Practica-1-SO
<br>
<img src="https://www.pngkey.com/png/detail/268-2688228_universidad-nacional-colombia-logo.png" width="230" alt="Logo Universidad Nacional de Colombia">
<br>
UNIVERSIDAD NACIONAL DE COLOMBIA 
<br>
Sistemas Operativos
<br>
2016707 – Grupo 2
<br><br>
Autores: 

- Nicolás Aguirre Velásquez (niaguirrev@unal.edu.co)
- Ever Nicolás Muñoz Cortés (evmunoz@unal.edu.co)
- Fabián David Mora Martinez (fmoram@unal.edu.co)

Docente: Cesar Augusto Pedraza Bonilla
<br><br>

---

## Contenido

- [1. Estructura del Proyecto](#1-estructura-del-proyecto)
  - [1.2 Estructura del Dataset](#12-estructura-del-dataset)
  - [1.3 Búsqueda y Criterios](#13-búsqueda-y-criterios)
- [2. Ejecución Local](#2-ejecución-local)
- [3. Instrucciones de Uso](#3-instrucciones-de-uso)
- [4. Ejemplos de Uso](#4-ejemplos-de-uso)

    
<br><br>

---


## 1. Estructura del Proyecto
``` txt
.
├───chemestry/
│   │
│   ├───dataset_1_1GB.csv                      # Dataset de 1.1GB (parte del dataset original)
│   ├───definitions.h                          # Archivo de cabecera con definiciones de estructuras (ej: SHM, estructuras de datos), constantes, etc.
│   ├───index_builder.c                        # Código para el programa que crea/actualiza la tabla hash (hash_index.bin).
│   ├───main.c                                 # Código del proceso principal. Encargado de la lógica de procesos (SHM, fork, wait, etc), y orquestación.
│   ├───MurmurHash2.c                          # Implementación de la función hash MurmurHash2.
│   ├───ui_process.c                           # Código del proceso UI. Gestiona la entrada/salida del usuario.
│   └───worker_process.c                       # Código del proceso Worker. Contiene la lógica principal de búsqueda en el archivo indexado.
```
Dataset: [MINI_SAVI Dataset en Hugging Face](https://huggingface.co/datasets/T-NOVA/MINI_SAVI)

<br><br>

---
#### 1.2 Estructura del Dataset

- product_name: Guarda el nombre completo o identificador de la reacción.

- product_hashisy: Guarda el identificador único del producto químico.

- product_smiles: Guarda la representación molecular del producto en formato SMILES.

- formula: Guarda la fórmula química molecular del producto.

- weight: Guarda el peso molecular del producto.

- rotbonds: Guarda el número de enlaces rotatorios.

- h_donors: Guarda el número de donantes de enlaces de hidrógeno.

- h_acceptors: Guarda el número de aceptores de enlaces de hidrógeno.

- complexity: Guarda una métrica de complejidad estructural del compuesto.

- product_qed_score: Guarda la puntuación QED (Quantitative Estimation of Drug-likeness) del producto.

- hydrogen_bond_center_count: Guarda el número de centros de enlace de hidrógeno.

- heavy_atoms: Guarda el número de átomos pesados (no-hidrógeno).

- ring_system_count: Guarda el número de sistemas de anillos.

- e_nesssr: Guarda un descriptor relacionado con el número de sistemas de anillos.

- e_nsssr: Guarda un descriptor relacionado con el complejidad del sistema de anillos.

- product_aroring_count: Guarda el número de anillos aromáticos.

- product_aliring_count: Guarda el número de anillos alifáticos.

- pains_filter_match_count: Guarda el número de coincidencias con filtros PAINS (indicando posibles problemas de ensayo).

- pains_filter_match_name: Guarda el nombre del filtro PAINS que coincidió (si aplica).

- transform_id: Guarda el identificador de la transformación (tipo de reacción) utilizada.

- r1_hashisy: Guarda el identificador único del reactivo 1.

- r1_smiles: Guarda la representación SMILES del reactivo 1.

- r2_hashisy: Guarda el identificador único del reactivo 2.

- r2_smiles: Guarda la representación SMILES del reactivo 2.
<br><br>

---
 #### 1.3 Búsqueda y Criterios
La búsqueda se centra en el atributo `product_smiles` porque actúa como el criterio principal y único para identificar una molécula. El SMILES es tomado como entrada para la función hash MurmurHash2 (implementada en el proyecto) para generar una clave numérica eficiente. Esta metodología se eligió específicamente para cumplir con el requisito de implementar una función de hashing propia, en lugar de utilizar el `product_hashisy` precalculado del dataset como clave directa.
 
<br><br>

---

## 2. Ejecución Local
Comandos para ejecutar el programa en la terminal de Linux:
``` txt
// Crear el archivo binario 
gcc index_builder.c MurmurHash2.c -o index_builder
./index_builder 

// Realizar las búsquedas 
gcc main.c worker_process.c ui_process.c MurmurHash2.c -o main  
./main
```

<br><br>

---

## 3. Instrucciones de Uso
El proceso ui_process presentará el siguiente menú. Debes ingresar el número de la opción deseada (1, 2, o 3).
```txt
--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: [VACÍO]
2. Realizar búsqueda
3. Salir
Selección:
```
- Selecciona la opción 1 e ingresa la cadena completa del `product_smiles`. Este valor se almacena como la clave actual.
- Selecciona la opción 2. El proceso UI envía la clave actual al proceso Worker. El Worker realiza la búsqueda indexada y devuelve el registro.
- Selecciona la opción 3 para terminar el sistema.
  
<br><br>

---

## 4. Ejemplos de Uso

> **IMPORTANTE:** Para fines de desarrollo, testing y gestión eficiente de recursos en el entorno local, la implementación utiliza un subconjunto (1.1 GB) del dataset original (1.8 GB).

### Ejemplo 1: 
product_smiles = O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC3=C[NH]C(=N3)CC4=CC=CC=C4
``` txt
#MAIN -> Memoria Compartida creada y adjunta.
#WORKER -> Tabla hash cargada en RAM (0.023 MB). Esperando comandos...
#UI -> Interfaz iniciada y conectada a Worker.

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: [VACÍO]
2. Realizar búsqueda
3. Salir
Selección: 1
Ingrese product_smiles (ej: O(C(=O)C2(CC1...)): O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC3=C[NH]C(=N3)CC4=CC=CC=C4

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC3=C[NH]C(=N3)CC4=CC=CC=C4
2. Realizar búsqueda
3. Salir
Selección: 2

Buscando 'O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC3=C[NH]C(=N3)CC4=CC=CC=C4'. Esperando resultado del Worker...
#WORKER -> Buscando: 'O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC3=C[NH]C(=N3)CC4=CC=CC=C4'...
#WORKER -> Búsqueda finalizada en 0.0000780 segundos.
Coincidencia encontrada:
"""4E583CDF5D62B693_2F298919F17BFE25_6031_UN""","""4E583CDF5D62B693""","""O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC3=C[NH]C(=N3)CC4=CC=CC=C4""","""C23H24N2O2""",360.4548,8,1,3,478.77,0.6795,4,27,4,4,4,3,1,0,"""""",6031,"""2BFCE7D1BAF62175""","""OC(=O)C2(CC1=CC=CC=C1)CCC2""","""04D56EC84B8DDF50""","""OCC1=C[NH]C(=N1)CC2=CC=CC=C2"""

Búsqueda finalizada.

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: O(C(=O)C2(CC1=CC=CC=C1)CCC2)CC3=C[NH]C(=N3)CC4=CC=CC=C4
2. Realizar búsqueda
3. Salir
Selección: 3
Comando de salida enviado al Worker. Terminando...
#WORKER -> Comando EXIT recibido. Terminando...
#MAIN -> Programa finalizado y recursos liberados.
#MAIN -> Desmontando y eliminando memoria compartida de mensajes...
```

### Ejemplo 2: 
product_smiles = O(C(=O)C1=CC(=NC=C1)[N]2C=CC=N2)CC(COC)(N)C3=CC=CC=C3
``` txt
#MAIN -> Memoria Compartida creada y adjunta.
#WORKER -> Tabla hash cargada en RAM (0.023 MB). Esperando comandos...
#UI -> Interfaz iniciada y conectada a Worker.

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: [VACÍO]
2. Realizar búsqueda
3. Salir
Selección: 1
Ingrese product_smiles (ej: O(C(=O)C2(CC1...)): O(C(=O)C1=CC(=NC=C1)[N]2C=CC=N2)CC(COC)(N)C3=CC=CC=C3

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: O(C(=O)C1=CC(=NC=C1)[N]2C=CC=N2)CC(COC)(N)C3=CC=CC=C3
2. Realizar búsqueda
3. Salir
Selección: 2

Buscando 'O(C(=O)C1=CC(=NC=C1)[N]2C=CC=N2)CC(COC)(N)C3=CC=CC=C3'. Esperando resultado del Worker...
#WORKER -> Buscando: 'O(C(=O)C1=CC(=NC=C1)[N]2C=CC=N2)CC(COC)(N)C3=CC=CC=C3'...
#WORKER -> Búsqueda finalizada en 0.0083960 segundos.
Coincidencia encontrada:
"""63D47E17B04E1796_40F41A67D06B361F_6031_UN""","""63D47E17B04E1796""","""O(C(=O)C1=CC(=NC=C1)[N]2C=CC=N2)CC(COC)(N)C3=CC=CC=C3""","""C19H20N4O3""",352.392,8,1,6,458.0652,0.702,6,26,3,3,3,3,0,0,"""""",6031,"""5B47C6A44B5336FA""","""OC(=O)C1=CC(=NC=C1)[N]2C=CC=N2""","""1BB3DCC39B3800E5""","""COCC(N)(CO)C1=CC=CC=C1"""

Búsqueda finalizada.

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: O(C(=O)C1=CC(=NC=C1)[N]2C=CC=N2)CC(COC)(N)C3=CC=CC=C3
2. Realizar búsqueda
3. Salir
Selección: 3
Comando de salida enviado al Worker. Terminando...
#WORKER -> Comando EXIT recibido. Terminando...
#MAIN -> Programa finalizado y recursos liberados.
#MAIN -> Desmontando y eliminando memoria compartida de mensajes...
```

### Ejemplo 3: 
product_smiles = CC1=C(C=C(C(=C1)C(=O)OCC2OCC3(NC2)CCC3)Cl)F
``` txt
#MAIN -> Memoria Compartida creada y adjunta.
#WORKER -> Tabla hash cargada en RAM (0.023 MB). Esperando comandos...
#UI -> Interfaz iniciada y conectada a Worker.

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: [VACÍO]
2. Realizar búsqueda
3. Salir
Selección: 1
Ingrese product_smiles (ej: O(C(=O)C2(CC1...)): CC1=C(C=C(C(=C1)C(=O)OCC2OCC3(NC2)CCC3)Cl)F

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: CC1=C(C=C(C(=C1)C(=O)OCC2OCC3(NC2)CCC3)Cl)F
2. Realizar búsqueda
3. Salir
Selección: 2

Buscando 'CC1=C(C=C(C(=C1)C(=O)OCC2OCC3(NC2)CCC3)Cl)F'. Esperando resultado del Worker...
#WORKER -> Buscando: 'CC1=C(C=C(C(=C1)C(=O)OCC2OCC3(NC2)CCC3)Cl)F'...
#WORKER -> Búsqueda finalizada en 0.0005610 segundos.
Coincidencia encontrada:
"""58643031D47EE42E_2E061AE2FAEA84DE_6031_UN""","""58643031D47EE42E""","""CC1=C(C=C(C(=C1)C(=O)OCC2OCC3(NC2)CCC3)Cl)F""","""C16H19ClFNO3""",327.7824,4,1,5,418.1951,0.9168,5,22,2,3,3,1,2,0,"""""",6031,"""5C3291E73640963B""","""CC1=C(C=C(C(=C1)C(=O)O)Cl)F""","""72348B05CCAA12E5""","""OCC1OCC2(NC1)CCC2"""

Búsqueda finalizada.

--- Sistema de búsqueda de moléculas ---
1. Ingresar criterio de búsqueda (product_smiles):
   Clave actual: CC1=C(C=C(C(=C1)C(=O)OCC2OCC3(NC2)CCC3)Cl)F
2. Realizar búsqueda
3. Salir
Selección: 3
Comando de salida enviado al Worker. Terminando...
#WORKER -> Comando EXIT recibido. Terminando...
#MAIN -> Programa finalizado y recursos liberados.
#MAIN -> Desmontando y eliminando memoria compartida de mensajes...
```

---

