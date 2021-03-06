find_program(DBUS_RUNNER_BIN dbus-test-runner)

#tst-buteo-import
add_executable(tst-buteo-import
    tst-buteo-import.cpp
)

target_link_libraries(tst-buteo-import
    Qt5::Core
    Qt5::DBus
    Qt5::Test
    Qt5::Contacts
    address-book-updater-lib
)

add_test(tst-buteo-import
    ${DBUS_RUNNER}
    --keep-env
    --task ${CMAKE_CURRENT_BINARY_DIR}/tst-buteo-import --wait-for=com.meego.msyncd
    --task ${CMAKE_CURRENT_SOURCE_DIR}/mock/mock-buteo.py --wait-for=com.canonical.pim -r --task-name=\"Buteo\"
    --task ${CMAKE_CURRENT_SOURCE_DIR}/mock/mock-galera.py -r --task-name=\"AddressBook Service\"
    --task ${CMAKE_CURRENT_SOURCE_DIR}/mock/mock-sync-monitor.py -r --task-name=\"Sync Monitor\"
)

set_tests_properties(tst-buteo-import PROPERTIES
    ENVIRONMENT ACCOUNTS=${CMAKE_CURRENT_BINARY_DIR}/accounts/full-database
)


#tst-buteo-import-no-accounts
add_executable(tst-buteo-import-no-accounts
    tst-buteo-import-no-accounts.cpp
)

target_link_libraries(tst-buteo-import-no-accounts
    Qt5::Core
    Qt5::DBus
    Qt5::Test
    Qt5::Contacts
    address-book-updater-lib
)

add_test(tst-buteo-import-no-accounts
    ${DBUS_RUNNER}
    --keep-env
    --task ${CMAKE_CURRENT_BINARY_DIR}/tst-buteo-import-no-accounts --wait-for=com.meego.msyncd
    --task ${CMAKE_CURRENT_SOURCE_DIR}/mock/mock-buteo.py --wait-for=com.canonical.pim -r --task-name=\"Buteo\"
    --task ${CMAKE_CURRENT_SOURCE_DIR}/mock/mock-galera.py -r --task-name=\"AddressBook Service\"
)

set_tests_properties(tst-buteo-import-no-accounts PROPERTIES
    ENVIRONMENT ACCOUNTS=${CMAKE_CURRENT_BINARY_DIR}/accounts/empty-database
)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/updater
)

add_subdirectory(accounts)
