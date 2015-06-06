import java.io.*;
import java.net.*;
import java.util.*;

public class Main{
    static ObjectOutputStream outputStream;
    static ObjectInputStream inputStream;
    public static void main(String[] args){
        System.out.println("Connecting to server...");
        Socket sock;
        try{
            sock = new Socket("45.50.5.238", 38007);
            System.out.println("Connected");

            inputStream = new ObjectInputStream(sock.getInputStream());
            outputStream = new ObjectOutputStream(sock.getOutputStream());

            ConnectMessage connect = new ConnectMessage("David Zero");
            outputStream.writeObject(connect);

            System.out.println("Starting a new game...");
            outputStream.writeObject(new CommandMessage(CommandMessage.Command.NEW_GAME));

            while(true){
                System.out.println("Attempting to read an incoming object");
                Message gameBoard = (Message)inputStream.readObject();

                if(gameBoard instanceof BoardMessage){
                    printBoard(((BoardMessage)gameBoard).getBoard());
                    makeMove();
                }
                else if(gameBoard instanceof ErrorMessage){
                    System.out.println("Error: " + ((ErrorMessage)gameBoard).getError());
                    System.exit(-1);
                }
                else{
                    System.out.println("Something went terribly wrong...");
                    System.exit(-1);
                }
            }
        }
        catch(Exception e){
            System.out.println("Exception: " + e.toString());
        }
    }

    static Scanner sc = new Scanner(System.in);
    static byte x,y;
    public static void makeMove(){
        sc.useDelimiter("(\\n)|,");
        System.out.print("Make a move: ");
        x = sc.nextByte();
        y = sc.nextByte();
        try{
            outputStream.writeObject(new MoveMessage(y,x));
        }
        catch(Exception e){
            System.out.println("Exception: " + e.toString());
            System.exit(-1);
        }
    }

    public static void printBoard(byte[][] board){
        for(int i = 0; i < board.length; i++){
            System.out.print("\t");
            for(int j = 0; j < board[i].length; j++){
                switch(board[i][j]){
                    case 0:
                        System.out.print("  .");
                        break;
                    case 1:
                        System.out.print("  X");
                        break;
                    case 2:
                        System.out.print("  O");
                        break;
                    default:
                        System.out.print("DAFUQ");
                }
            }
            System.out.println();
        }
        System.out.println();
    }
}
