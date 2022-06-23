CXXFLAGS = -std=c++11 -Wall -Wextra -Wundef # -Werror

targets = eliza

all : $(targets)

# % is the wildcard for an implicit rule.
# $< is the first prerequisite.  In this case, $(target).c.
# $@ is the target.
% : %.cpp
	g++ $(CXXFLAGS) $< -o $@

.PHONY : clean
clean:
	rm $(targets)

