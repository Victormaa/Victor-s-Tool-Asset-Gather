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
    public float accelerationScale = 3f; // 原为1.0f
    public float drag = 0.5f;
    public float sensitive = 2.0f;          // 原为0.1f

    void Start()
    {
        rb = GetComponent<Rigidbody>();
        if (rb == null)
        {
            Debug.LogError("未找到Rigidbody组件，请在物体上添加Rigidbody！");
        }

        udpClient = new UdpClient(port);
        remoteEP = new IPEndPoint(IPAddress.Any, 0);
        Debug.Log("UDP 已启动，监听端口: " + port);
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
            // 计算移动方向（基于摄像机视角）
            Vector3 moveDirection = AlignMoveDirection(-ax, -ay);
            Vector3 acceleration = moveDirection * accelerationScale;

            rb.AddForce(acceleration * sensitive, ForceMode.Acceleration);

            return;
        }

        if (udpClient.Available > 0)
        {
            // 接收 ESP32 发送的数据
            byte[] data = udpClient.Receive(ref remoteEP);
            string text = Encoding.UTF8.GetString(data);
            Debug.Log("收到数据: " + text);

            // 回传确认信息
            byte[] ack = Encoding.UTF8.GetBytes("ACK:" + Time.time);
            udpClient.Send(ack, ack.Length, remoteEP);

            // 数据解析
            string[] parts = text.Split(',');
            if (parts.Length == 6)
            {
                float ax = float.Parse(parts[0]);
                float ay = float.Parse(parts[1]);
                float az = float.Parse(parts[2]);
                float gx = float.Parse(parts[3]);
                float gy = float.Parse(parts[4]);
                float gz = float.Parse(parts[5]);

                // TODO: 在这里处理姿态
            }
        }
    }
    private Vector3 AlignMoveDirection(float ax, float ay)
    {
        // 获取摄像机的正前方向（忽略Y轴，防止角色向上飞）
        Vector3 cameraForward = Camera.main.transform.forward;
        cameraForward.y = 0;
        cameraForward.Normalize();

        // 获取摄像机的正右方向（同样忽略Y轴）
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
