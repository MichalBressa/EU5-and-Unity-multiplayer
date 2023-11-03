using System;
using System.Linq;
using System.Threading.Tasks;

using System.Net;
using System.Net.Sockets;
using System.Text;
//using System.Text.Json;
using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;


namespace LabTask1
{
    internal class Program
    {
        static int lastAssignedGlobalID = 12; //I arbitrarily start at 12 so it’s easy to see if it’s working
        static Dictionary<int, byte[]> gameState = new Dictionary<int, byte[]>();
        static List<IPEndPoint> connectedClients = new List<IPEndPoint>();

        static void Main(string[] args)
        {
            int recv;
            byte[] data = new byte[2048];

            //IPEndPoint ipep = new IPEndPoint(IPAddress.Parse("127.0.0.1"), 9050);
            IPEndPoint ipep = new IPEndPoint(IPAddress.Parse("10.0.74.148"), 9050); 
            //our server IP. This is set to local (127.0.0.1) on socket 9050. If 9050 is firewalled, you might want to try another!

            Socket newsock = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp); 
            //make a socket using UDP. The parameters passed are enums used by the constructor of Socket to configure the socket.

            newsock.Bind(ipep); //bind the socket to our given IP
            Console.WriteLine("Socket open..."); //if we made it this far without any networking errors, it’s a good start!

            IPEndPoint sender = new IPEndPoint(IPAddress.Any, 0);

            EndPoint Remote = (EndPoint)(sender);

            //First parameter is the data, 2nd is packet size, 3rd is any flags we want, and 4th is destination client.
            bool IPisInList;

            while (true)
            {
                data= new byte[2048];
                recv = newsock.ReceiveFrom(data, ref Remote); //recv is now a byte array containing whatever just arrived from the client

                IPisInList = false;
                IPEndPoint senderIPEndPoint = (IPEndPoint)Remote;

                foreach (IPEndPoint ep in connectedClients)
                {
                    if (senderIPEndPoint.ToString().Equals(ep.ToString())) IPisInList = true;
                }
                if (!IPisInList)
                {
                    connectedClients.Add(senderIPEndPoint);
                    Console.WriteLine("A new client just connected. There are now " + connectedClients.Count + " clients.");
                }

                //Console.WriteLine("Message received from " + Remote.ToString()); //this will show the client’s unique id
                //Console.WriteLine(Encoding.ASCII.GetString(data, 0, recv));

                string messageRecieved = Encoding.ASCII.GetString(data, 0, recv);

                //is this packet a UID request?
                if (messageRecieved.Contains("I need a UID for local object:"))
                {
                    //Console.WriteLine(messageRecieved.Substring(messageRecieved.IndexOf(':')));

                    //parse the string into an into to get the local ID
                    int localObjectNumber = Int32.Parse(messageRecieved.Substring(messageRecieved.IndexOf(':') + 1));
                    //assign the ID
                    string returnVal = ("Assigned UID:" + localObjectNumber + ";" + lastAssignedGlobalID++);
                    //Console.WriteLine(returnVal);
                    newsock.SendTo(Encoding.ASCII.GetBytes(returnVal), Encoding.ASCII.GetBytes(returnVal).Length, SocketFlags.None, Remote);
                }
                else if (messageRecieved.Contains("Object data;"))
                {
                    //get the global id from the packet
                    //Console.WriteLine(messageRecieved);

                    string globalId = messageRecieved.Split(';')[1];
                    int intId = Int32.Parse(globalId);
                    if (gameState.ContainsKey(intId))
                    { 
                        //if true, we're already tracking the object
                        gameState[intId] = data; 
                        //data being the original bytes of the packet
                    }
                    else //the object is new to the game
                    {
                        gameState.Add(intId, data);
                    }
                }

                // send data back to clients
                foreach (IPEndPoint ep in connectedClients)
                {
                    //Console.WriteLine("Sending gamestate to " + ep.ToString());
                    if (ep.Port != 0)
                    {
                        foreach (KeyValuePair<int, byte[]> kvp in gameState)
                        {
                            newsock.SendTo(kvp.Value, kvp.Value.Length, SocketFlags.None, ep);
                        }
                    }
                }
            }
        }
    }
}
