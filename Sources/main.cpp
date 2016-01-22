#include "Urho3DAll.h"

using namespace Urho3D;
class MyApp : public Application
{
public:
    SharedPtr<Scene> scene1;
    SharedPtr<Node> cameraNode;
    SharedPtr<Camera> camera;
    SharedPtr<Viewport> vp;

    #define INSTANCES 24
    SharedPtr<Node> instancesNode;
    SharedPtr<Material> instancesMaterial;
    SharedPtr<StaticModel> instancesSm;
    PODVector<Matrix4> instancesTransfoms;
    Vector3 rotDir[INSTANCES];

    SharedPtr<DebugHud> debugHud;
    
    
    MyApp(Context* context) :
        Application(context)
    {
    }
    virtual void Setup()
    {
        // Called before engine initialization. engineParameters_ member variable can be modified here
        engineParameters_["WindowTitle"] = "noname";
		engineParameters_["FullScreen"] = false;
		engineParameters_["Headless"] = false;
		engineParameters_["WindowWidth"] = 1280;
		engineParameters_["WindowHeight"] = 720;
		engineParameters_["LogName"] = GetTypeName() + ".log";
		engineParameters_["VSync"] = false;
		engineParameters_["FrameLimiter"] = false;
		engineParameters_["WorkerThreads"] = true;
		engineParameters_["RenderPath"] = "CoreData/RenderPaths/Forward.xml";
		engineParameters_["Sound"] = false;
    }
    virtual void Start()
    {
        // Called after engine initialization. Setup application & subscribe to events here
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MyApp, HandleUpdate));
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MyApp, HandleKeyDown));

        CreateDebugUI();
        LoadScene();
        FindCamera();
        FindInstances();

        for (int i = 0; i < INSTANCES; i++) 
        {
            float x = Random(-1.0f, 1.0f);
            float y = Random(-1.0f, 1.0f);
            float z = Random(-1.0f, 1.0f);
            rotDir[i] = Vector3(x, y, z).Normalized();
        }
    }
    virtual void Stop()
    {
        // Perform optional cleanup after main loop has terminated
    }

    void LoadScene()
    {
        scene1 = SharedPtr<Scene>(new Scene(context_));
        File sceneFile(context_, GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/Scene1.xml", FILE_READ);
        scene1->LoadXML(sceneFile);
    }

    void FindCamera() 
    {
        if (scene1) 
        {
            cameraNode = scene1->GetChild("cameraNode");
            if (cameraNode)
            {
                camera = cameraNode->GetComponent<Camera>();
                if (camera)
                {
                    vp = SharedPtr<Viewport>(new Viewport(context_, scene1, camera));
                    Renderer* renderer = GetSubsystem<Renderer>();
                    renderer->SetViewport(0, vp);
                    
                }
            }
        }
    }

    void FindInstances()
    {
        if (scene1)
        {
            instancesNode = scene1->GetChild("instancesNode");
            if (instancesNode)
            {
                instancesSm = instancesNode->GetComponent<StaticModel>();
                instancesMaterial = instancesSm->GetMaterial(0);
                instancesTransfoms.Resize(INSTANCES);
            }
        }
    }

    void CreateDebugUI()
    {

        ResourceCache* cache = GetSubsystem<ResourceCache>();
        XMLFile* xmlFile = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
        Engine* engine = GetSubsystem<Engine>();

        if (!engine) return;

        // Create console
        Console* console = engine->CreateConsole();
        console->SetDefaultStyle(xmlFile);
        console->GetBackground()->SetOpacity(0.8f);

        debugHud = engine->CreateDebugHud();
        debugHud->SetDefaultStyle(xmlFile);
        //debugHud->SetMode(DEBUGHUD_SHOW_ALL | DEBUGHUD_SHOW_ALL_MEMORY);
    }

    void ToggleDebugUI()
    {
        if (debugHud)
            debugHud->ToggleAll();
    }

    void HandleKeyDown(StringHash eventType, VariantMap& eventData)
    {
        using namespace KeyDown;
        // Check for pressing ESC. Note the engine_ member variable for convenience access to the Engine object
        int key = eventData[P_KEY].GetInt();
        if (key == KEY_ESC)
            engine_->Exit();

        if (key == KEY_F1)
            ToggleDebugUI();
    }

    void HandleUpdate(StringHash eventType, VariantMap& eventData)
    {
        using namespace Update;
        float timeStep = eventData[P_TIMESTEP].GetFloat();
        
        static float rotation = 0.0;
        static float t = 0.0;
        // Calc transformation
        
        // We need keep actual BB for our static model with instances to avoid discarding all instances at onece by camera frustrum
        BoundingBox bb;
        bb.Merge(instancesNode->GetWorldPosition());

        for (int i = 0; i < INSTANCES; i++) 
        {
            Matrix4 translation;
            Vector3 pos = Vector3(-10 + i, 1 + i / 3, 15);
            
            // Merge current instance position into BB
            bb.Merge(pos);

            translation.SetTranslation(pos);
            
            const float  speed = 10.0;
            rotation += speed * timeStep;

            Quaternion q;
            q.FromEulerAngles(rotation * rotDir[i].x_, rotation * rotDir[i].y_, rotation * rotDir[i].z_);
            Matrix3 rot = q.RotationMatrix();
            
            // All instnces by default wes saved at zero world origin
            // So at first we rotate theys and then translate to wpos
            Matrix4 finalTransform = translation * Matrix4(rot);
            instancesTransfoms[i] = finalTransform;
        }

        // Put all transformation into shader, 
        // if we needed just placed instances somewhere in scene, there is no needed do this every frame
        if (instancesMaterial) 
        {

            Model* model = instancesSm->GetModel();
            model->SetBoundingBox(bb);
            
            VectorBuffer binaryBuffer;
            
            /*
            for (int i = 0; i < INSTANCES; i++) 
            {
                binaryBuffer.WriteMatrix4(instancesTransfoms[i]);
            }
            */

            binaryBuffer.SetData(&instancesTransfoms[0], sizeof(Matrix4) * INSTANCES);
            
            Variant instArray(binaryBuffer);
            instancesMaterial->SetShaderParameter("InstancesTransforms", instArray);
        }
    }
};
URHO3D_DEFINE_APPLICATION_MAIN(MyApp)