# VCalc Fuzzer

This Python script will automatically generate a test case for the VCalc assignment.

## Requirements

- Python == 3.8.10

## Usage

The main Python script is `fuzzer.py`. Run `python fuzzer.py -h` to see the following usage text and description:

```
usage: fuzzer.py [-h] [--seed SEED] config name

Procedurely generate a test case for VCalc

positional arguments:
  config       path to configuration file
  name         name of test case to generate (creates name.in, name.out)

optional arguments:
  -h, --help   show this help message and exit
  --seed SEED  random number generator seed
```

## Configuration

A default configuration file is provided as `config.json`.

The following is a break-down of each of the configurable options:

```
{
    "generation-options": {
        "max-expression-recursion-depth": 3,    // Controls the maximum length of expressions
        "max-conditionals-loops": 10            // Limits the number of conditionals and loops allowed
    },
    "properties": {
        "max-range-length": 256,                // Maximum length of a range, and by consequence the maximum length of a vector
        "id-length": {
            "min": 1,                           // Minimum length of an identifier
            "max": 20                           // Maximum length of an identifier
        }
    },
    "expression-weights": {                     // Weights of expressions. 
                                                // The higher the value, the more likely a particular type of expression will be generated.
                                                // The weights must be positive integers.
                                                // The weights do not need to sum to 100.
        "+": 80,                                // Generate an addition operation
        "-": 80,                                // Generate a subtraction operation
        "*": 80,                                // Generate a multiplication operation
        "/": 80,                                // Generate a division operation
        "<": 20,                                // Generate a less than comparison
        ">": 20,                                // Generate a greater than comparison
        "==": 20,                               // Generate an equality comparion
        "!=": 20,                               // Generate an inequality comparison
        "()": 30,                               // Generate an expression enclosed in parenthesis
        "range": 30,                            // Generate a range expression
        "index": 30,                            // Generate an index expression
        "generator": 30,                        // Generate a generator
        "filter": 30,                           // Generate a filter
        "id": 100,                              // Generate an identifier (that has been declared)
        "int": 100,                             // Generate an integer literal
        "max-int": 1,                           // Generate the largest integer literal: 2147483647
        "min-int": 1                            // Generate the smallest integer: (0 - 2147483647 - 1)
    },
    "statement-weights": {                      // Weights of statements.
                                                // The direct analogue of expression weights but for statements.
        "int-declaration": 50,                  // Generate an integer declaration statement.
        "vec-declaration": 50,                  // Generate a vector declaration statement.
        "assignment": 50,                       // Generate an assignment statement for modifying variables.
        "conditional": 50,                      // Generate a conditional statement.
        "loop": 50,                             // Generate a loop.
        "print": 50                             // Generate a print statement.
    },
    "block-terminate-probability": 0.2          // Probability of terminating the current block of code instead of generating a statement.
}
```

Note that the above is a commented version of the default configuration used for explanatory purposes only. Configuration files do not support comments.
