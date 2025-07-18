CC = gcc
CFLAGS = -Wall -pedantic # -fsanitize=address
LIBS = -pthread -lncurses

TARGET = quantum
BUILD_DIR = build

SRCS = 	main.c \
		inputs.c \
		display.c \
		chat.c \
		listes.c \

OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

all: $(TARGET)
	@echo "✅ Build completed successfully!"
	@echo "🚀 You can now launch \033[4;32m./quantum\033[0m"

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "🔧 Compiling $< ..."
	@$(CC) $(CFLAGS) -c $< -o $@ $(LIBS) 

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	@echo "🔨 Linking $(TARGET) ..."
	@$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

re: fclean all

clean:
	@echo "🧹 Cleaning objects..."
	@rm -rf $(TARGET) $(BUILD_DIR)

fclean: clean	
	@echo "🧹 Cleaning exe..."
	@rm -f reccord.txt
	@rm -f $(TARGET)