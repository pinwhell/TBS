if(TBS_TESTS)
	add_subdirectory(doctest)
endif()

if(TBS_USE_ETL)
	if(NOT TARGET etl)
		add_subdirectory(etl)
	endif()

	add_compile_definitions(etl INTERFACE ETL_NO_STL)
endif()