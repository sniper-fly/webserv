CXX = clang++
INCLUDE = -I"./googletest/include" -I"./include"
CXXFLAGS = -g -Wall -Wextra -Werror -pthread -std=c++98 $(INCLUDE) -MMD -MP

ifdef fsanitize
CXXFLAGS += -fsanitize=address
endif

SRCS = $(shell find ./src/ -type f -name '*.cpp')

OBJ_DIR = objects/
OBJS = $(addprefix $(OBJ_DIR), $(SRCS:.cpp=.o))
DEPENDS = $(OBJS:.o=.d)

COVFILES = $(OBJS:.o=.gcda) $(OBJS:.o=.gcno) cov_test.info coverageFiltered.info

NAME = webserv

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJ_DIR)%.o: %.cpp
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPENDS)

clean:
	rm -rf $(OBJS) $(DEPENDS)

fclean: clean
	rm -rf $(NAME) $(COVFILES) ./cov_test

re: fclean all

ifdef cov
CXX = g++
CXXFLAGS += -ftest-coverage -fprofile-arcs -lgcov
endif

coverage:
	make re cov=1
	./$(NAME)
	lcov -c -b . -d . -o cov_test.info
	lcov -r cov_test.info "*/c++/*" -o coverageFiltered.info
	genhtml coverageFiltered.info -o cov_test
	make clean

open:
	xdg-open cov_test/index.html

.PHONY: all clean fclean re coverage open
