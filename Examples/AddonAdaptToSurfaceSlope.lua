-- Lua Addon for AddonAdaptToSurfaceSlope

while not GetForceAdaptRotation() do
    print("Forcing slope adaptation ON...")
    SetForceAdaptRotation(true)
    if GetForceAdaptRotation() then
        print("Slope adaptation is now ON!")
    else
        -- Wait for the constructor hook to capture the instance
        print("Waiting for AddonAdaptToSurfaceSlope instance...")
        os.execute("sleep 1")
    end
end

-- Turn it off after 5 seconds as a demo
os.execute("sleep 5")
print("Turning slope adaptation OFF.")
SetForceAdaptRotation(false)
print("Slope adaptation is now OFF:", not GetForceAdaptRotation())