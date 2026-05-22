# Especificaciones del Hardware de Pruebas

> **Nota:** Complete esta plantilla con las especificaciones reales del equipo utilizado para las pruebas.

---

## Procesador (CPU)

| Especificación             | Valor                        |
|--------------------------- |----------------------------- |
| **Modelo**                 | *[Ej: Intel Core i7-12700K]* |
| **Arquitectura**           | *[Ej: x86_64]*               |
| **Núcleos físicos**        | *[Ej: 12]*                   |
| **Núcleos lógicos (hilos)**| *[Ej: 20]*                   |
| **Frecuencia base**        | *[Ej: 3.60 GHz]*             |
| **Frecuencia turbo máx.**  | *[Ej: 5.00 GHz]*             |
| **TDP**                    | *[Ej: 125W]*                 |

---

## Memoria Caché

| Nivel  | Tamaño por núcleo       | Tamaño total           | Asociatividad       |
|------- |------------------------ |----------------------- |-------------------- |
| **L1 datos**      | *[Ej: 48 KB]*  | *[Ej: 576 KB]*        | *[Ej: 12-way]*      |
| **L1 instrucciones** | *[Ej: 32 KB]* | *[Ej: 384 KB]*     | *[Ej: 8-way]*       |
| **L2**            | *[Ej: 1.25 MB]* | *[Ej: 12 MB]*        | *[Ej: 10-way]*      |
| **L3 (compartida)** | —             | *[Ej: 25 MB]*        | *[Ej: 16-way]*      |

---

## Memoria RAM

| Especificación     | Valor                          |
|------------------- |------------------------------- |
| **Capacidad total**| *[Ej: 32 GB]*                  |
| **Tipo**           | *[Ej: DDR5]*                   |
| **Frecuencia**     | *[Ej: 4800 MHz]*               |
| **Canales**        | *[Ej: Dual-channel]*           |
| **Latencia (CAS)** | *[Ej: CL40]*                  |

---

## Sistema Operativo

| Especificación         | Valor                            |
|----------------------- |--------------------------------- |
| **Sistema operativo**  | *[Ej: Windows 11 Pro 23H2]*     |
| **Kernel**             | *[Ej: 10.0.22631]*              |
| **Arquitectura SO**    | *[Ej: 64-bit]*                  |

---

## Compilador y Herramientas

| Herramienta     | Versión                              |
|---------------- |------------------------------------- |
| **GCC**         | *[Ej: gcc (MinGW-W64) 13.2.0]*      |
| **OpenMP**      | *[Ej: 4.5]*                         |
| **Make**        | *[Ej: GNU Make 4.4.1]*              |
| **Python**      | *[Ej: Python 3.11.5]*               |
| **matplotlib**  | *[Ej: 3.8.0]*                       |

---

## Comandos para Obtener Especificaciones

### Linux

```bash
# Información del CPU
lscpu

# Información detallada del CPU
cat /proc/cpuinfo | head -30

# Caché del CPU
getconf -a | grep CACHE

# Memoria RAM
free -h

# Versión del kernel
uname -a

# Versión de GCC
gcc --version

# Soporte OpenMP
echo | gcc -fopenmp -dM -E - | grep OPENMP
```

### Windows (PowerShell)

```powershell
# Información del CPU
Get-WmiObject -Class Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed

# Memoria RAM
Get-WmiObject -Class Win32_PhysicalMemory | Select-Object Capacity, Speed, MemoryType

# Caché del CPU
Get-WmiObject -Class Win32_CacheMemory | Select-Object Purpose, InstalledSize

# Versión de GCC
gcc --version

# Versión del sistema
winver
```

---

## Notas Adicionales

*Registre aquí cualquier configuración especial del sistema, como:*
- *Hyperthreading habilitado/deshabilitado*
- *Modo de energía (alto rendimiento / balanceado)*
- *Procesos en segundo plano relevantes*
- *Configuración de BIOS/UEFI modificada*
- *Temperaturas durante las pruebas*
