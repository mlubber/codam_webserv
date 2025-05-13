# Name
NAME				=	webserv

# Directories
SRC_DIR				=	sources/
BLD_DIR				=	build/
OBJ_DIR				=	./build/objects/
INC					=	-I ./headers

# Complier and Flags
CPP					=	c++
CPPFLAGS			=	-Wall -Wextra -Werror -std=c++20
RM					=	rm -f

# Source Files
MAIN_DIR			=	$(SRC_DIR)main.cpp \
						$(SRC_DIR)signals.cpp \
						$(SRC_DIR)utils.cpp

PARSING_DIR			=	$(SRC_DIR)parsing/Configuration.cpp \
						$(SRC_DIR)parsing/ValidationConfigFile.cpp \
						$(SRC_DIR)parsing/Request.cpp

CLASS_DIR			=	$(SRC_DIR)classes/Client.cpp \
						$(SRC_DIR)classes/Server.cpp \
						$(SRC_DIR)classes/Host.cpp

REQ_DIR				=	$(SRC_DIR)requests/routing.cpp

CGI_DIR				=	$(SRC_DIR)cgi/cgi.cpp \
						$(SRC_DIR)cgi/cgi_parent.cpp \
						$(SRC_DIR)cgi/cgi_child.cpp \
						$(SRC_DIR)cgi/cgi_child_setup.cpp

# Concatenate all source files
SRCS				=	$(MAIN_DIR) $(PARSING_DIR) $(CLASS_DIR) $(REQ_DIR) $(CGI_DIR)

# Object creation
OBJS 				=	$(patsubst $(SRC_DIR)%.cpp,$(OBJ_DIR)%.o,$(SRCS))

# Make call
all: 					$(NAME)

# Build program
$(NAME):				$(OBJS)
						@echo "Building $(NAME)"
						@$(CPP) $(CPPFLAGS) $(INC) -o $(NAME) $(OBJS)

# Compile object files
$(OBJ_DIR)%.o: 			$(SRC_DIR)%.cpp
						@echo "Compiling $< for $(NAME)"
						@mkdir -p $(@D)
						@$(CPP) $(CPPFLAGS) $(INC) -c $< -o $@

# Clean up
clean:
						@echo "Cleaning..."
						@$(RM) -r $(OBJ_DIR)
						@$(RM) -r $(BLD_DIR)

fclean: 				clean
						@echo "Removing binaries..."
						@$(RM) $(NAME)
						@echo "All clean"

re: fclean all

.PHONY: all clean fclean re