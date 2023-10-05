# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/src/freeglut"
  "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/cmake-build-debug/glut-prefix/src/glut-build"
  "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/cmake-build-debug/glut-prefix"
  "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/cmake-build-debug/glut-prefix/tmp"
  "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/cmake-build-debug/glut-prefix/src/glut-stamp"
  "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/cmake-build-debug/glut-prefix/src"
  "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/cmake-build-debug/glut-prefix/src/glut-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/david/OneDrive/Documents/SzGrafika/SzamitogepesGrafikaBME/hazi1/cmake-build-debug/glut-prefix/src/glut-stamp/${subDir}")
endforeach()
