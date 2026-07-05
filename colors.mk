ifndef COLORS_MK
COLORS_MK = 1

BOLD    := \033[1m
RED     := \033[1;31m
GREEN   := \033[1;32m
YELLOW  := \033[1;33m
BLUE    := \033[1;34m
CYAN    := \033[1;36m
MAGENTA := \033[1;35m
RESET   := \033[0m

ifneq ($(NO_COLOR)$(CI),)
BOLD    :=
RED     :=
GREEN   :=
YELLOW  :=
BLUE    :=
CYAN    :=
MAGENTA :=
RESET   :=
endif

endif
