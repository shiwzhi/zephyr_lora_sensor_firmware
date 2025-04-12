cmake_minimum_required(VERSION 3.20.0)

add_library(lbm_zephyr)

target_sources(lbm_zephyr PRIVATE 
    SWL2001/lbm_lib/smtc_modem_core/smtc_ralf/src/ralf_sx126x.c
    SWL2001/lbm_lib/smtc_modem_core/smtc_ral/src/ral_sx126x.c
    SWL2001/lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/src/sx126x.c
    SWL2001/lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/src/sx126x_lr_fhss.c
    SWL2001/lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/src/lr_fhss_mac.c
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem.c
    SWL2001/lbm_lib/smtc_modem_core/modem_utilities/modem_core.c
    SWL2001/lbm_lib/smtc_modem_core/lorawan_api/lorawan_api.c
    SWL2001/lbm_lib/smtc_modem_core/lorawan_manager/lorawan_send_management.c
    SWL2001/lbm_lib/smtc_modem_core/radio_planner/src/radio_planner.c
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element/soft_se.c
    SWL2001/lbm_lib/smtc_modem_core/modem_supervisor/modem_supervisor_light.c
    SWL2001/lbm_lib/smtc_modem_core/modem_supervisor/modem_tx_protocol_manager.c
    SWL2001/lbm_lib/smtc_modem_core/modem_utilities/modem_event_utilities.c
    SWL2001/lbm_lib/smtc_modem_core/lorawan_manager/lorawan_join_management.c
    SWL2001/lbm_lib/smtc_modem_core/modem_utilities/fifo_ctrl.c
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_duty_cycle.c
    SWL2001/lbm_lib/smtc_modem_core/lorawan_manager/lorawan_dwn_ack_management.c
    SWL2001/lbm_lib/smtc_modem_core/lorawan_manager/lorawan_cid_request_management.c
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/lr1mac_core.c
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/smtc_real.c
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem_crypto/smtc_modem_crypto.c
    SWL2001/lbm_lib/smtc_modem_core/lorawan_packages/lorawan_certification/lorawan_certification.c
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/lr1_stack_mac_layer.c
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element/cmac.c
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem_crypto/soft_secure_element/aes.c
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem_test.c
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/services/smtc_lbt.c
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/lr1mac_utilities.c
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src/region_cn_470_rp_1_0.c

    ./smtc_modem_hal.c
    ./ral_sx126x_bsp.c
    ./sx126x_hal.c
)

target_include_directories(lbm_zephyr PUBLIC
    SWL2001/lbm_lib
    SWL2001/lbm_lib/smtc_modem_api
    SWL2001/lbm_lib/smtc_modem_hal
    SWL2001/lbm_lib/smtc_modem_core/logging
    SWL2001/lbm_lib/smtc_modem_core/smtc_ralf/src
    SWL2001/lbm_lib/smtc_modem_core/smtc_ral/src
    SWL2001/lbm_lib/smtc_modem_core/radio_drivers/sx126x_driver/src
    SWL2001/lbm_lib/smtc_modem_core/lorawan_manager
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem_crypto/smtc_secure_element
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/smtc_real/src
    SWL2001/lbm_lib/smtc_modem_core/radio_planner/src
    SWL2001/lbm_lib/smtc_modem_core/modem_supervisor
    SWL2001/lbm_lib/smtc_modem_core/modem_utilities
    SWL2001/lbm_lib/smtc_modem_core/lorawan_packages/lorawan_certification
    SWL2001/lbm_lib/smtc_modem_core/lorawan_api
    SWL2001/lbm_lib/smtc_modem_core/lr1mac/src/services
    SWL2001/lbm_lib/smtc_modem_core/smtc_modem_crypto
    SWL2001/lbm_lib/smtc_modem_core
    SWL2001/lbm_lib/smtc_modem_core/lr1mac
)

target_link_libraries(lbm_zephyr PUBLIC zephyr)
target_compile_definitions(lbm_zephyr PRIVATE SX126X=1 SX1262=1 RP2_103=1 NUMBER_OF_STACKS=1 REGION_CN_470_RP_1_0=1)