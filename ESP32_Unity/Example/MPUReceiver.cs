using UnityEngine;
using System.Net;
using System.Net.Sockets;
using System.Text;

public class MPUReceiver : MonoBehaviour
{
    public bool isUseMPU = true;
    UdpClient udpClient;
    int port = 5001;
    IPEndPoint remoteEP;


    private Rigidbody rb;
    public float accelerationScale = 3f; // ԭΪ1.0f
    public float drag = 0.5f;
    public float sensitive = 2.0f;          // ԭΪ0.1f

    void Start()
    {
        rb = GetComponent<Rigidbody>();
        if (rb == null)
        {
            Debug.LogError("δ�ҵ�Rigidbody������������������Rigidbody��");
        }

        udpClient = new UdpClient(port);
        remoteEP = new IPEndPoint(IPAddress.Any, 0);
        Debug.Log("UDP �������������˿�: " + port);
    }

    void Update()
    {
        if (!isUseMPU)
        {
            float ax;
            float ay;
            float az;
            ax = Input.GetAxis("Horizontal");
            ay = Input.GetAxis("Vertical");
            az = 0;
            // �����ƶ����򣨻���������ӽǣ�
            Vector3 moveDirection = AlignMoveDirection(-ax, -ay);
            Vector3 acceleration = moveDirection * accelerationScale;

            rb.AddForce(acceleration * sensitive, ForceMode.Acceleration);

            return;
        }

        if (udpClient.Available > 0)
        {
            // ���� ESP32 ���͵�����
            byte[] data = udpClient.Receive(ref remoteEP);
            string text = Encoding.UTF8.GetString(data);
            Debug.Log("�յ�����: " + text);

            // �ش�ȷ����Ϣ
            byte[] ack = Encoding.UTF8.GetBytes("ACK:" + Time.time);
            udpClient.Send(ack, ack.Length, remoteEP);

            // ���ݽ���
            string[] parts = text.Split(',');
            if (parts.Length == 6)
            {
                float ax = float.Parse(parts[0]);
                float ay = float.Parse(parts[1]);
                float az = float.Parse(parts[2]);
                float gx = float.Parse(parts[3]);
                float gy = float.Parse(parts[4]);
                float gz = float.Parse(parts[5]);

                // TODO: �����ﴦ����̬
            }
        }
    }
    private Vector3 AlignMoveDirection(float ax, float ay)
    {
        // ��ȡ���������ǰ���򣨺���Y�ᣬ��ֹ��ɫ���Ϸɣ�
        Vector3 cameraForward = Camera.main.transform.forward;
        cameraForward.y = 0;
        cameraForward.Normalize();

        // ��ȡ����������ҷ���ͬ������Y�ᣩ
        Vector3 cameraRight = Camera.main.transform.right;
        cameraRight.y = 0;
        cameraRight.Normalize();

        return cameraForward * -ay + cameraRight * -ax;
    }
    void OnApplicationQuit()
    {
        udpClient.Close();
    }
}
