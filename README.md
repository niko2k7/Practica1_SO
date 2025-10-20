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
  - [1.3 Búsqueda y Criterios](#13-busqueda-y-criterios)
- [2. Requisitos de Instalación](#2-requisitos-de-instalación)
- [3. Ejecución Local](#3-ejecucion-local)
- [4. Instrucciones de Uso](#4-instrucciones-de-uso)
- [5. Ejemplos de Uso](#5-ejemplos-de-uso)
- [6. Tecnologías Utilizadas](#6-tecnologías-utilizadas)

    
<br><br>

---


## 1. Estructura del Proyecto
``` txt
.
├───chemestry/
│   │
│   ├───dataset_1_1GB.csv                      # Dataset de 1.1GB
│   ├───definitions.h                          # Archivo de cabecera con definiciones de estructuras (ej: SHM, estructuras de datos), constantes, etc.
│   ├───index_builder.c                        # Código para el programa que crea/actualiza la tabla hash (hash_index.bin).
│   ├───main.c                                 # Código del proceso principal. Encargado de la lógica de procesos (SHM, fork, wait, etc), y orquestación.
│   ├───MurmurHash2.c                          # Implementación de la función hash MurmurHash2.
│   ├───ui_process.c                           # Código del proceso UI. Gestiona la entrada/salida del usuario.
│   └───worker_process.c                       # Código del proceso Worker. Contiene la lógica principal de búsqueda en el archivo indexado.
```
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
## 2. Requisitos de Instalación

<br><br>

---

## 3. Ejecución Local

br><br>

---

## 4. Instrucciones de Uso

<br><br>

---

## 5. Ejemplos de Uso


<br><br>

---

## 6. Tecnologías Utilizadas

Principales tecnologías, frameworks y librerías utilizadas en el proyecto.

#### Backend
- Python
- FastAPI
- Uvicorn
- NetworkX
- Matplotlib

#### Frontend
- React
- Vite
- TypeScript
- Tailwind CSS
- React Router DOM
- Radix UI
- Lucide React icons

#### Otros
- Git
- VSCode
- npm
- pydantic
- jsdom
- cytoscape
- postcss
- Figma
- Builder.io
<br><br>

---
