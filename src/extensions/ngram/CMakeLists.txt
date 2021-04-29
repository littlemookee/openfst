file(GLOB HEADER_FILES ../../include/fst/extensions/ngram/*.h)
message(STATUS "${HEADER_FILES}")

include_directories(../../include; ../../../../kaldi-crypt/src ../../../../usr/include)

link_directories(C:/proj/openfst/x64/Release; C:/proj/usr/lib.msvc140-x64; "C:/Program Files (x86)/Guardant/SDK7/Lib/windows/x64")

add_definitions(-DNOMINMAX -DFST_NO_DYNAMIC_LINKING -DOPENFSTSTATIC)

add_library(fstngram
    bitmap-index.cc 
    ngram-fst.cc 
    nthbit.cc
    ${HEADER_FILES}
)

target_link_libraries(fstngram
    fst
)

set_target_properties(fstngram PROPERTIES
    SOVERSION "7"
    FOLDER ngram
)

install(TARGETS fstngram 
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION lib
)

add_library(ngram_fst MODULE
    bitmap-index.cc 
    ngram-fst.cc 
    nthbit.cc
)

set_target_properties(ngram_fst PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS true
    FOLDER ngram/modules
)

target_link_libraries(ngram_fst
    libfst
    GrdAPI64
)

install(TARGETS ngram_fst
    LIBRARY DESTINATION lib/fst
)

