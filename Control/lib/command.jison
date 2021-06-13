%lex
%%

\s                      return 'WHITESPACE'
\b[0-9]+"mm"\b                return 'DIST'
\b([0-9]|[1-8][0-9]|9[0-9]|[12][0-9]{2}|3[0-4][0-9]|35[0-9])"deg"\b  return 'HEADING'
\b([0-9]|[1-8][0-9]|9[0-9]|100)"%"   return 'PERCENTAGE'
\bmove\b                  return 'MOVE'
\bpstop\b                 return 'PSTOP'
\bstop\b                  return 'STOP'
\bcharge\sto\b                return 'CHARGETO'
\btelemetry\sreset\b      return 'TELERST'
<<EOF>>                 return 'EOF'
.                       return 'INVALID'

/lex

%start command

%%

command
    : expr EOF
    ;

expr
    : MOVE WHITESPACE DIST WHITESPACE HEADING WHITESPACE PERCENTAGE
        {   
            var inDist = String($3).substr(0, ((String($3).length) - 2));
            var inHdg = String($5).substr(0, ((String($5).length) - 3));
            var inSpd = String($7).substr(0, ((String($7).length) - 1));
            mode = 1;
            reqDistance = Number(inDist);
            reqHeading = Number(inHdg);
            reqSpeed = Number(inSpd);
            reqCharge = 0;
            send_data();
            updateCommandBuffer();
            command_id++;
        }
    | STOP
        {
            mode = 0;
            reqDistance = 0;
            reqHeading = 0;
            reqSpeed = 0;
            reqCharge = 0;
            send_data();
            command_id = 0;
            updateCommandBuffer()
        }
    | PSTOP
        {
            mode = 1;
            reqDistance = 0;
            reqHeading = 0;
            reqSpeed = 0;
            reqCharge = 0;
            send_data();
            updateCommandBuffer();
            command_id++;
        }
    | CHARGETO WHITESPACE PERCENTAGE
        {
            mode = 1;
            reqDistance = 0;
            reqHeading = 0;
            reqSpeed = 0;

            var inChrg = String($3).substr(0, ((String($3).length) - 1));
            reqCharge = Number(inChrg);

            send_data();
            updateCommandBuffer();
            command_id++;
        }
    | TELERST
        {mode = 1;
            reqDistance = 0;
            reqHeading = 0;
            reqSpeed = 0;
            reqCharge = 0;
            send_data();
            updateCommandBuffer();
            command_id++;}
    ;

