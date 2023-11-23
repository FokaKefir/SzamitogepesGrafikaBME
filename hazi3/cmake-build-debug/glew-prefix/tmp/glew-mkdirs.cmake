# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/src/glew"
  "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix/src/glew-build"
  "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix"
  "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix/tmp"
  "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix/src/glew-stamp"
  "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix/src"
  "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix/src/glew-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix/src/glew-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/david/OneDrive/Documents/Egyetem/SzGrafika/SzamitogepesGrafikaBME/hazi3/cmake-build-debug/glew-prefix/src/glew-stamp${cfgdir}") # cfgdir has leading slash
endif()
