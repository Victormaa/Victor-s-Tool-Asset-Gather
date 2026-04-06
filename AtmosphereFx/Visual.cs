using UnityEngine;
using System.Collections;

public class Visual : MonoBehaviour 
{
	public float DontChangeThis = 0.0f;
	
	void OnDrawGizmosSelected() 
	{
        Gizmos.color = new Color(1, 1, 1, 0.5F);
        Gizmos.DrawCube(transform.position, new Vector3(15, DontChangeThis, 15));
    }
}