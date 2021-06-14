%lex
%%

\s                      return 'whitespace'
\b[0-9]+"mm"\b                return 'distance'
\b([0-9]|[1-8][0-9]|9[0-9]|[12][0-9]{2}|3[0-4][0-9]|35[0-9])"deg"\b  return 'heading_angle'
\b([0-9]|[1-8][0-9]|9[0-9]|100)"%"   return 'percentage'
\b[0-9]+"s"\b           return 'stop_duration'
\bmove\b                  return 'move'
\bpstop\b                 return 'pstop'
\bstop\b                  return 'stop'
\bhelp\b                return 'help'
\bcharge\sto\b                return 'charge_to'
\btelemetry\sreset\b      return 'telemetry_reset'
<<EOF>>                 return 'EOF'
.                       return 'invalid_command'

/lex

%start command

%%

command
    : expr EOF
    ;

expr
    : move whitespace distance whitespace heading_angle whitespace percentage
        {   
            var inDist = Number(String($3).substr(0, ((String($3).length) - 2)));
            var inHdg = Number(String($5).substr(0, ((String($5).length) - 3)));
            var inSpd = Number(String($7).substr(0, ((String($7).length) - 1)));
            moveCmd(inDist,inHdg,inSpd);
        }
    | stop
        {
            stpCmd();
        }
    | pstop whitespace stop_duration
        {
            var inStpDur = Number(String($3).substr(0, ((String($3).length) - 1)));
            pstpCmd(inStpDur);
        }
    | charge_to whitespace percentage
        {
            var inChrg = Number(String($3).substr(0, ((String($3).length) - 1)));
            chrgCmd(inChrg);
        }
    | telemetry_reset
        {   
            telRst();
        }
    | help
        {
            printHelpDetails();
        }
    ;

