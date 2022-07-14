use std::fs::File;
use std::io::Read;
use std::env;
use std::fmt::{Display, Formatter};

enum Token {
    NoToken,
    Plus,
    Minus,
    Star,
    Slash,
    IntLit(i64)
}

impl Display for Token {
    fn fmt(&self, f: &mut Formatter) -> std::fmt::Result {
        match self {
            Token::NoToken => write!(f, "Token None"),
            Token::Plus => write!(f, "Token +"),
            Token::Minus => write!(f, "Token -"),
            Token::Star => write!(f, "Token *"),
            Token::Slash => write!(f, "Token /"),
            Token::IntLit(val) => write!(f, "Token intlit, value {}", val)
        }
    }
}

struct Compiler {
    line: i64,
    putback: char,
    in_data: Vec<u8>
}

impl Compiler {
    fn new(filename: &str) -> Self {
        let mut f = File::open(filename).expect("no file found");
        let metadata = std::fs::metadata(filename).expect("unable to read metadata");
        let mut buffer = vec![0; metadata.len() as usize];
        f.read(&mut buffer).expect("buffer overflow");

        Self {
            line: 1,
            putback: '\n',
            in_data: buffer.to_owned()
        }
    }

    fn run(&mut self) {
        let mut t: Token = Token::NoToken;

        while self.scan(&mut t) {
            println!("{}", t);
        }
    }

    fn scan(&mut self, t: &mut Token) -> bool {
        let mut c = self.skip();

        match c {
            '\0' => {
                return false;
            },
            '+' => {
                *t = Token::Plus;
            },
            '-' => {
                *t = Token::Minus;
            },
            '*' => {
                *t = Token::Star;
            },
            '/' => {
                *t = Token::Slash;
            },
            _ => {
                if c.is_digit(10) {
                    *t = Token::IntLit(self.scanint(&mut c));
                } else {
                    println!("Unrecognized character {} on line {}", c, self.line);
                    return false;
                }
            }
        };

        return true;
    }

    fn skip(&mut self) -> char {
        let mut c: char = self.next();

        while (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') {
            c = self.next();
        }

        c
    }

    fn scanint(&mut self, c: &mut char) -> i64 {
        let mut k: i64 = match c.to_digit(10) {
            Some(val) => val.into(),
            None => -1
        };
        let mut val = 0;

        while k >= 0 {
            val = val * 10 + k;
            *c = self.next();
            k = match c.to_digit(10) {
                Some(val) => val.into(),
                None => -1
            };
        }

        self.putback_val(*c);
        val.into()
    }

    fn next(&mut self) -> char {
        let c: char;

        if self.putback != '\0' {
            c = self.putback;
            self.putback = '\0';
            return c;
        }

        c = self.read();

        if c == '\n' {
            self.line += 1;
        }

        c
    }

    fn putback_val(&mut self, c: char) {
        self.putback = c;
    }

    fn read(&mut self) -> char {
        if self.in_data.len() == 0 {
            return '\0';
        }

        let c: char = self.in_data[0] as char;
        self.in_data = self.in_data[1..].to_vec();
        c
    }
}

fn usage(prog: String) {
    println!("Usage: {} infile", prog);
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() != 2 {
        usage(args[0].clone());
        return;
    }

    let mut compiler = Compiler::new(&args[1]);
    compiler.run();
}