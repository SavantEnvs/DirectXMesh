# mayhem/ubsan_scope.cmake — injected into upstream's CMake (CMAKE_PROJECT_DirectXMesh_INCLUDE) for the
# fuzz build only. Turns off two UBSan checks on the specific library sources where they fire on
# intended, benign patterns; ASan and every other UBSan check stay on and halting everywhere, and both
# checks stay on in every other source file and in the harnesses. Flags go on the compile line, so
# ccache hashes them (no -fsanitize-ignorelist=).
#
# alignment (-fsanitize=alignment):
#   DirectXMeshClean.cpp     Clean(): scratch buffer is bool faceSeen[3*nFaces] then uint32_t ids[] then
#                            index_t indicesNew[] in one uint8_t[]; ids/indicesNew are misaligned whenever
#                            nFaces*3 is not a multiple of 4 (Clean.cpp:195,272-283,315-347,382).
#   DirectXMeshValidate.cpp  ValidateNoBowties(): same packed bool+index_t scratch layout
#                            (Validate.cpp:336-341,360).
#   DirectXMeshVBReader.cpp  VBReader::Impl::Read(): reads each element at the caller's arbitrary
#                            AlignedByteOffset/stride through XMLoad* (DirectXMath inlines, e.g.
#                            DirectXPackedVector.inl:1570 via VBReader.cpp:330-352).
#   DirectXMeshVBWriter.cpp  VBWriter::Impl::Write(): the same for XMStore* (VBWriter.cpp:299-340).
# nonnull-attribute (-fsanitize=nonnull-attribute):
#   DirectXMeshletGenerator.cpp  ComputeMeshlets(): memcpy(dst, v.data(), 0) from an empty std::vector,
#                            whose data() is nullptr (DirectXMeshletGenerator.cpp:552-553).
set(_dxm "${CMAKE_CURRENT_SOURCE_DIR}/DirectXMesh")
set_source_files_properties(
  "${_dxm}/DirectXMeshClean.cpp" "${_dxm}/DirectXMeshValidate.cpp"
  "${_dxm}/DirectXMeshVBReader.cpp" "${_dxm}/DirectXMeshVBWriter.cpp"
  PROPERTIES COMPILE_OPTIONS "-fno-sanitize=alignment")
set_source_files_properties("${_dxm}/DirectXMeshletGenerator.cpp"
  PROPERTIES COMPILE_OPTIONS "-fno-sanitize=nonnull-attribute")
