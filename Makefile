# Name
NAME				=	webserv

# Directories
SRC_DIR				=	sources/
BLD_DIR				=	build/
OBJ_DIR				=	./build/objects/
INC					=	-I ./headers

# Complier and Flags
CPP					=	c++
CPPFLAGS			=	-Wall -Wextra -Werror #-std=c++98
RM					=	rm -f

# Source Files
MAIN_DIR			=	$(SRC_DIR)main.cpp

CLASS_DIR			=	$(SRC_DIR)classes/Server.cpp

# Concatenate all source files
SRCS				=	$(MAIN_DIR) $(CLASS_DIR)

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