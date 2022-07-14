use std::io::Read;
use std::fs::File;

enum Token {
    Plus,
    Minus,
    Star,
    Slash,
    IntLit(i64)
}

impl std::fmt::Display for Token {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Token::Plus => write!(f, "Token +"),
            Token::Minus => write!(f, "Token -"),
            Token::Star => write!(f, "Token *"),
            Token::Slash => write!(f, "Token /"),
            Token::IntLit(val) => write!(f, "Token intlit, value {}", val)
        }
    }
}

struct Compiler {
    putback: char,
    line: i64,
    in_data: Vec<u8>,
    index: usize
}

fn chrpos(s: &str, c: char) -> i64 {
    match s.chars().position(|ch| ch == c) {
        Some(val) => val as i64,
        None => -1
    }
}

impl Compiler {
    fn next(&mut self) -> Option<char> {
        let c: Option<char>;

        if self.putback != '\0' {
            c = Some(self.putback);
            self.putback = '\0';
            return c;
        }
        
        c = self.read();

        if let Some('\n') = c {
            self.line += 1;
        }

        return c;
    }

    fn read(&mut self) -> Option<char> {
        if self.index < self.in_data.len() {
            let c: char = self.in_data[self.index] as char;
            self.index += 1;
            Some(c)
        } else {
            None
        }
    }

    fn skip(&mut self) -> Option<char> {
        let c: Option<char> = self.next();

        match c {
            Some(val) => {
                let mut vval = val;

                while (vval == ' ') || (vval == '\t') || (vval == '\n') || (vval == '\r') {
                    let new_char: Option<char> = self.next();

                    match new_char {
                        Some(new_val) => vval = new_val,
                        None => return None
                    }
                }

                Some(vval)
            },
            None => c
        }
    }

    fn scan(&mut self, token: &mut Token) -> bool {
        let c: Option<char> = self.skip();

        match c {
            Some(cval) => match cval {
                '+' => {
                    *token = Token::Plus;
                    true
                },
                '-' => {
                    *token = Token::Minus;
                    true
                },
                '*' => {
                    *token = Token::Star;
                    true
                },
                '/' => {
                    *token = Token::Slash;
                    true
                },
                _ => {
                    if cval.is_digit(10) {
                        *token = Token::IntLit(self.scanint(cval));
                        true
                    } else {
                        panic!("Unrecognized character {} on line {}", c.unwrap_or('\0'), self.line);
                    }
                }
            },
            None => false
        }
    }

    fn scanint(&mut self, c: char) -> i64 {
        let mut k: i64 = chrpos("0123456789", c);
        let mut val: i64 = 0;
        let mut ch: Option<char> = Some(c);

        while k >= 0 {
            val = val * 10 + k;
            ch = self.next();
            k = chrpos("0123456789", ch.unwrap_or('\0'));
        }

        self.putback = ch.unwrap_or('\0');
        return val;
    }

    fn scanfile(&mut self) {
        let mut token: Token = Token::Plus;

        while self.scan(&mut token) {
            println!("{}", token);
        }
    }

    fn new(filename: &str) -> Self {
        let mut f = File::open(filename).expect("no file found");
        let metadata = std::fs::metadata(filename).expect("unable to read metadata");
        let mut buffer = vec![0; metadata.len() as usize];
        f.read(&mut buffer).expect("buffer overflow");

        Self {
            line: 1,
            putback: '\n',
            in_data: buffer.to_owned(),
            index: 0
        }
    }
}

fn usage(prog: String) {
    println!("Usage: {} infile", prog);
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() != 2 {
        usage(args[0].clone());
        return;
    }

    let mut compiler = Compiler::new(&args[1]);
    compiler.scanfile();
}