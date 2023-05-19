# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ_DRIVER ap_pwrseq.c)
zephyr_library_sources_ifdef(CONFIG_AP_PWRSEQ_DRIVER ap_pwrseq_sm.c)

