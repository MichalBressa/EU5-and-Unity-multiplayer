using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;

public class NetworkManager : MonoBehaviour
{

    public struct UdpState
    {
        public UdpClient u;
        public IPEndPoint e;
    }
    static UdpClient client;
    static IPEndPoint ep;
    static UdpState state;

    public List<NetworkGameObject> netObjects;
    [SerializeField] GameObject networkAvatar;
    string receiveString = "";
    public List<NetworkGameObject> worldState;

    // Start is called before the first frame update
    void Start()
    {
        netObjects = new List<NetworkGameObject>();
        netObjects.AddRange(GameObject.FindObjectsOfType<NetworkGameObject>());
        Debug.Log("starting up");

        worldState = new List<NetworkGameObject>();
        worldState.AddRange(GameObject.FindObjectsOfType<NetworkGameObject>());

        client = new UdpClient();
        ep = new IPEndPoint(IPAddress.Parse("10.0.74.148"), 9050); // endpoint where server is listening (testing localy)
        client.Connect(ep);

        string myMessage = "Bruuuh";
        byte[] array = Encoding.ASCII.GetBytes(myMessage);
        client.Send(array, array.Length);

        client.BeginReceive(ReceiveAsyncCallback, state);

        RequestUIDs();

        StartCoroutine(SendNetworkUpdates());
        StartCoroutine(updateWorldState());
    }

    void RequestUIDs()
    {

        //List<NetworkGameObject> netObjects = new List<NetworkGameObject>();
        //netObjects.AddRange(GameObject.FindObjectsOfType<NetworkGameObject>());
        foreach (NetworkGameObject netObject in netObjects)
        {
            if (netObject.isLocallyOwned && netObject.uniqueNetworkID == 0)
            {
                string myMessage = "I need a UID for local object:" + netObject.localID;
                byte[] array = Encoding.ASCII.GetBytes(myMessage);
                client.Send(array, array.Length);
            }
        }
    }

    int GetGlobalIDFromPacket(String packet)
    {
        return Int32.Parse(packet.Split(';')[1]);
    }

    public static String betweenStrings(String text, String start, String end)
    {
        int p1 = text.IndexOf(start) + start.Length;
        int p2 = text.IndexOf(end, p1);

        if (end == "") return (text.Substring(p1));
        else return text.Substring(p1, p2 - p1);
    }

    
    void ReceiveAsyncCallback(IAsyncResult result)
    {
        byte[] receiveBytes = client.EndReceive(result, ref ep); //get the packet
        receiveString = Encoding.ASCII.GetString(receiveBytes); //decode the packet
        //Debug.Log("Received " + receiveString + " from " + ep.ToString()); //display the packet
        //client.BeginReceive(ReceiveAsyncCallback, state); //self-callback, meaning this loops infinitely

        if (receiveString.Contains("Assigned UID:"))
        {
            int parseFrom = receiveString.IndexOf(':');
            int parseTo = receiveString.LastIndexOf(';');

            //we need to parse the string from the server back into ints to work with
            int localID = Int32.Parse(betweenStrings(receiveString, ":", ";"));
            int globalID = Int32.Parse(receiveString.Substring(receiveString.IndexOf(";") + 1));

            Debug.Log("Got assignment: " + localID + " local to: " + globalID + " global");

            foreach (NetworkGameObject netObject in netObjects)
            {
                //if the local ID sent by the server matches this game object
                if (netObject.localID == localID)
                {
                    Debug.Log(localID + " : " + globalID);
                    //the global ID becomes the server-provided value
                    netObject.uniqueNetworkID = globalID;
                }
            }
        }

        //continue to loop
        client.BeginReceive(ReceiveAsyncCallback, state);
    }

    IEnumerator SendNetworkUpdates()
    {
        while (true)
        {
            foreach (NetworkGameObject netObject in netObjects)
            {
                if (netObject.isLocallyOwned)
                {
                    client.Send(netObject.toPacket(), netObject.toPacket().Length);
                }
            }

            yield return new WaitForSeconds(0.1f);
        }
    }

    IEnumerator updateWorldState()
    {
        while (true)
        {
            //read in the current world state as all network game objects in the scene
            worldState = new List<NetworkGameObject>();
            worldState.AddRange(GameObject.FindObjectsOfType<NetworkGameObject>());

            //cache the recieved packet string - we'll use that later to suspend the couroutine until it changes
            string previousRecieveString = receiveString;

            //if it's an object update, process it, otherwise skip
            if (receiveString.Contains("Object data;"))
            {
                //we'll want to know if an object with this global id is already in the game world
                bool objectIsAlreadyInWorld = false;

                //we'll also want to exclude any invalid packets with a bad global id
                if (GetGlobalIDFromPacket(receiveString) != 0)
                {
                    //for every networked gameobject in the world
                    foreach (NetworkGameObject ngo in worldState)
                    {
                        //if it's unique ID matches the packet, update it's position from the packet
                        if (ngo.uniqueNetworkID == GetGlobalIDFromPacket(receiveString))
                        {
                            //only update it if we don't own it - you might want to try disabling and seeing the effect
                            if (!ngo.isLocallyOwned)
                            {
                                ngo.fromPacket(receiveString);

                            }
                            //if we have any uniqueID matches, our object is in the world
                            objectIsAlreadyInWorld = true;
                        }

                    }

                    //if it's not in the world, we need to spawn it
                    if (!objectIsAlreadyInWorld)
                    {
                        Debug.Log("Object not in the world");
                        GameObject otherPlayerAvatar = Instantiate(networkAvatar);
                        //update its component properties from the packet
                        otherPlayerAvatar.GetComponent<NetworkGameObject>().uniqueNetworkID = GetGlobalIDFromPacket(receiveString);
                        otherPlayerAvatar.GetComponent<NetworkGameObject>().fromPacket(receiveString);
                    }
                }

            }

            //wait until the incoming string with packet data changes then iterate again

            yield return new WaitUntil(() => !receiveString.Equals(previousRecieveString));
        }
    }


}
