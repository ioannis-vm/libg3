#==============================================================================
# 
#        OpenSees -- Open System For Earthquake Engineering Simulation
#                Pacific Earthquake Engineering Research Center
#
#==============================================================================
# add_library(OPS_Material_YieldSurface OBJECT)

# target_include_directories(OPS_Material_YieldSurface PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# add_subdirectory(./evolution/)
# add_subdirectory(./plasticHardeningMaterial/)
# add_subdirectory(./yieldSurfaceBC/)

target_link_libraries(OpenSeesRT PRIVATE OPS_Material_YieldSurface)

target_sources(OPS_Runtime PRIVATE
    "TclModelBuilderYS_EvolutionCommand.cpp"
    "TclModelBuilderYSPlasticMaterialCommand.cpp"
    "TclModelBuilderYieldSurfaceBCCommand.cpp"
    "TclModelBuilderYS_SectionCommand.cpp"
)


