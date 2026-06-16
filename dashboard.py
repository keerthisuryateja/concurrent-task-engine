import redis
import time
import sys

def main():
    print("Connecting to Redis...")
    try:
        r = redis.Redis(host='localhost', port=6379, decode_responses=True)
        r.ping()
        print("Connected! Starting dashboard...")
        print("-" * 40)
        
        while True:
            val = r.get("total_tasks_completed")
            count = int(val) if val is not None else 0
                
            print(f"\rTasks Completed: {count} / 30", end="", flush=True)
            
            if count >= 30:
                print("\n\nAll tasks finished. Exiting.")
                break
            
            time.sleep(0.1)
            
    except redis.ConnectionError:
        print("\nError: Redis server not found.")
    except KeyboardInterrupt:
        print("\n\nStopped by user.")

if __name__ == "__main__":
    main()