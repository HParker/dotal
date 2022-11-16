const char *prelude = "fn print_i8(x: i8) do\n"
                      "  found := i8\n"
                      "  digit := i8\n"
                      "\n"
                      "  digit = x / 100\n"
                      "  if digit > 0 || found do\n"
                      "    found = 1\n"
                      "    send(.Console/write, digit + 48)\n"
                      "  end\n"
                      "  x = x - digit * 100\n"
                      "\n"
                      "  digit = x / 10\n"
                      "  if digit > 0 || found do\n"
                      "    found = 1\n"
                      "    send(.Console/write, digit + 48)\n"
                      "  end\n"
                      "  x = x - digit * 10\n"
                      "\n"
                      "  send(.Console/write, x + 48)\n"
                      "end\n";
